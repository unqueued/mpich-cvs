/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce = PMPI_Reduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce  MPI_Reduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce as PMPI_Reduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce
#define MPI_Reduce PMPI_Reduce

/* This is the default implementation of reduce. The algorithm is:
   
   Algorithm: MPI_Reduce

   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see 
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html ).
   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is 
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta + 
           n.(1+(p-1)/p).gamma


   For short messages, user-defined ops, and count < pof2, we use a
   binomial tree algorithm for both short and long messages. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma


   We use the binomial tree algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky. 

   Possible improvements: 

   End Algorithm: MPI_Reduce
*/

/* begin:nested */
/* not declared static because a machine-specific function may call this one 
   in some cases */
int MPIR_Reduce ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    int root, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Reduce";
    MPI_Status status;
    int        comm_size, rank, is_commutative, type_size, pof2, rem, newrank;
    int        mask, relrank, source, lroot, *cnts, *disps, i, j, send_idx=0;
    int        mpi_errno = MPI_SUCCESS, recv_idx, last_idx=0, newdst;
    int    dst, send_cnt, recv_cnt, newroot, newdst_tree_root,
        newroot_tree_root; 
    MPI_User_function *uop;
    MPI_Aint   true_lb, true_extent, extent; 
    void       *tmp_buf;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPIU_THREADPRIV_DECL;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    MPIU_CHKLMEM_DECL(4);
    
    if (count == 0) return MPI_SUCCESS;

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* set op_errno to 0. stored in perthread structure */
    MPIU_THREADPRIV_FIELD(op_errno) = 0;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
        /* get the function by indexing into the op table */
        uop = MPIR_Op_table[op%16 - 1];
    }
    else {
        MPID_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
        
#ifdef HAVE_CXX_BINDING            
            if (op_ptr->language == MPID_LANG_CXX) {
                uop = (MPI_User_function *) op_ptr->function.c_function;
		is_cxx_uop = 1;
	    }
	    else
#endif
        if ((op_ptr->language == MPID_LANG_C))
            uop = (MPI_User_function *) op_ptr->function.c_function;
        else
            uop = (MPI_User_function *) op_ptr->function.f77_function;
    }
    
    /* Create a temporary buffer */

    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb, &true_extent);  
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)),
			mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
	MPIU_CHKLMEM_MALLOC(recvbuf, void *, 
			    count*(MPIR_MAX(extent,true_extent)), 
			    mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    MPID_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
        
    if ((count*type_size > MPIR_REDUCE_SHORT_MSG) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        /* do a reduce-scatter followed by gather to root. */

        rem = comm_size - pof2;

        /* In the non-power-of-two case, all odd-numbered
           processes of rank < 2*rem send their data to
           (rank-1). These odd-numbered processes no longer
           participate in the algorithm until the very end. The
           remaining processes form a nice power-of-two. 

           Note that in MPI_Allreduce we have the even-numbered processes
           send data to odd-numbered processes. That is better for
           non-commutative operations because it doesn't require a
           buffer copy. However, for MPI_Reduce, the most common case
           is commutative operations with root=0. Therefore we want
           even-numbered processes to participate the computation for
           the root=0 case, in order to avoid an extra send-to-root
           communication after the reduce-scatter. In MPI_Allreduce it
           doesn't matter because all processes must get the result. */
        
        if (rank < 2*rem) {
            if (rank % 2 != 0) { /* odd */
                mpi_errno = MPIC_Send(recvbuf, count, 
                                      datatype, rank-1,
                                      MPIR_REDUCE_TAG, comm);
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                
                /* temporarily set the rank to -1 so that this
                   process does not pariticipate in recursive
                   doubling */
                newrank = -1; 
            }
            else { /* even */
                mpi_errno = MPIC_Recv(tmp_buf, count, 
                                      datatype, rank+1,
                                      MPIR_REDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE);
 		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                
                /* do the reduction on received data. */
                /* This algorithm is used only for predefined ops
                   and predefined ops are always commutative. */
#ifdef HAVE_CXX_BINDING
                if (is_cxx_uop) {
                    (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                    count,
                                                    datatype,
                                                    uop ); 
                }
                else 
#endif
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);

                /* change the rank */
                newrank = rank / 2;
            }
        }
        else  /* rank >= 2*rem */
            newrank = rank - rem;
        
        /* for the reduce-scatter, calculate the count that
           each process receives and the displacement within
           the buffer */

        /* We allocate these arrays on all processes, even if newrank=-1,
           because if root is one of the excluded processes, we will
           need them on the root later on below. */
	MPIU_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
	MPIU_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");
        
        if (newrank != -1) {
            for (i=0; i<(pof2-1); i++) 
                cnts[i] = count/pof2;
            cnts[pof2-1] = count - (count/pof2)*(pof2-1);
            
            disps[0] = 0;
            for (i=1; i<pof2; i++)
                disps[i] = disps[i-1] + cnts[i-1];
            
            mask = 0x1;
            send_idx = recv_idx = 0;
            last_idx = pof2;
            while (mask < pof2) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 : newdst + rem;
                
                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    send_idx = recv_idx + pof2/(mask*2);
                    for (i=send_idx; i<last_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<send_idx; i++)
                        recv_cnt += cnts[i];
                }
                else {
                    recv_idx = send_idx + pof2/(mask*2);
                    for (i=send_idx; i<recv_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<last_idx; i++)
                        recv_cnt += cnts[i];
                }
                
/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                      send_cnt, recv_cnt, last_idx);
*/
                /* Send data from recvbuf. Recv into tmp_buf */ 
                mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                          disps[send_idx]*extent,
                                          send_cnt, datatype,  
                                          dst, MPIR_REDUCE_TAG, 
                                          (char *) tmp_buf +
                                          disps[recv_idx]*extent,
                                          recv_cnt, datatype, dst,
                                          MPIR_REDUCE_TAG, comm,
                                          MPI_STATUS_IGNORE);
 		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                
                /* tmp_buf contains data received in this step.
                   recvbuf contains data accumulated so far */
                
                /* This algorithm is used only for predefined ops
                   and predefined ops are always commutative. */
#ifdef HAVE_CXX_BINDING
                if (is_cxx_uop) {
                    (*MPIR_Process.cxx_call_op_fn)((char *) tmp_buf +
                                                   disps[recv_idx]*extent,
                                                   (char *) recvbuf + 
                                                   disps[recv_idx]*extent, 
                                                   recv_cnt, datatype, uop);
                }
                else 
#endif
                    (*uop)((char *) tmp_buf + disps[recv_idx]*extent,
                           (char *) recvbuf + disps[recv_idx]*extent, 
                           &recv_cnt, &datatype);
                
                /* update send_idx for next iteration */
                send_idx = recv_idx;
                mask <<= 1;

                /* update last_idx, but not in last iteration
                   because the value is needed in the gather
                   step below. */
                if (mask < pof2)
                    last_idx = recv_idx + pof2/mask;
            }
        }

        /* now do the gather to root */
        
        /* Is root one of the processes that was excluded from the
           computation above? If so, send data from newrank=0 to
           the root and have root take on the role of newrank = 0 */ 

        if (root < 2*rem) {
            if (root % 2 != 0) {
                if (rank == root) {    /* recv */
                    /* initialize the arrays that weren't initialized */
                    for (i=0; i<(pof2-1); i++) 
                        cnts[i] = count/pof2;
                    cnts[pof2-1] = count - (count/pof2)*(pof2-1);
                    
                    disps[0] = 0;
                    for (i=1; i<pof2; i++)
                        disps[i] = disps[i-1] + cnts[i-1];
                    
                    mpi_errno = MPIC_Recv(recvbuf, cnts[0], datatype,  
                                          0, MPIR_REDUCE_TAG, comm,
                                          MPI_STATUS_IGNORE);
                    newrank = 0;
                    send_idx = 0;
                    last_idx = 2;
                }
                else if (newrank == 0) {  /* send */
                    mpi_errno = MPIC_Send(recvbuf, cnts[0], datatype,  
                                          root, MPIR_REDUCE_TAG, comm);
                    newrank = -1;
                }
                newroot = 0;
            }
            else newroot = root / 2;
        }
        else
            newroot = root - rem;

        if (newrank != -1) {
            j = 0;
            mask = 0x1;
            while (mask < pof2) {
                mask <<= 1;
                j++;
            }
            mask >>= 1;
            j--;
            while (mask > 0) {
                newdst = newrank ^ mask;

                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 : newdst + rem;
                /* if root is playing the role of newdst=0, adjust for
                   it */
                if ((newdst == 0) && (root < 2*rem) && (root % 2 != 0))
                    dst = root;
                
                /* if the root of newdst's half of the tree is the
                   same as the root of newroot's half of the tree, send to
                   newdst and exit, else receive from newdst. */

                newdst_tree_root = newdst >> j;
                newdst_tree_root <<= j;
                
                newroot_tree_root = newroot >> j;
                newroot_tree_root <<= j;

                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    /* update last_idx except on first iteration */
                    if (mask != pof2/2)
                        last_idx = last_idx + pof2/(mask*2);
                    
                    recv_idx = send_idx + pof2/(mask*2);
                    for (i=send_idx; i<recv_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<last_idx; i++)
                        recv_cnt += cnts[i];
                }
                else {
                    recv_idx = send_idx - pof2/(mask*2);
                    for (i=send_idx; i<last_idx; i++)
                        send_cnt += cnts[i];
                    for (i=recv_idx; i<send_idx; i++)
                        recv_cnt += cnts[i];
                }
                
                if (newdst_tree_root == newroot_tree_root) {
                    /* send and exit */
                    /* printf("Rank %d, send_idx %d, send_cnt %d, last_idx %d\n", newrank, send_idx, send_cnt, last_idx);
                       fflush(stdout); */
                    /* Send data from recvbuf. Recv into tmp_buf */ 
                    mpi_errno = MPIC_Send((char *) recvbuf +
                                          disps[send_idx]*extent,
                                          send_cnt, datatype,  
                                          dst, MPIR_REDUCE_TAG, 
                                          comm);
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                    break;
                }
                else {
                    /* recv and continue */
                    /* printf("Rank %d, recv_idx %d, recv_cnt %d, last_idx %d\n", newrank, recv_idx, recv_cnt, last_idx);
                       fflush(stdout); */
                    mpi_errno = MPIC_Recv((char *) recvbuf +
                                          disps[recv_idx]*extent,
                                          recv_cnt, datatype, dst,
                                          MPIR_REDUCE_TAG, comm,
                                          MPI_STATUS_IGNORE);
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                }
                
                if (newrank > newdst) send_idx = recv_idx;
                
                mask >>= 1;
                j--;
            }
        }
    }

    else {  /* use a binomial tree algorithm */ 
    
    /* This code is from MPICH-1. */

    /* Here's the algorithm.  Relative to the root, look at the bit pattern in 
       my rank.  Starting from the right (lsb), if the bit is 1, send to 
       the node with that bit zero and exit; if the bit is 0, receive from the
       node with that bit set and combine (as long as that node is within the
       group)
       
       Note that by receiving with source selection, we guarentee that we get
       the same bits with the same input.  If we allowed the parent to receive 
       the children in any order, then timing differences could cause different
       results (roundoff error, over/underflows in some cases, etc).
       
       Because of the way these are ordered, if root is 0, then this is correct
       for both commutative and non-commutitive operations.  If root is not
       0, then for non-commutitive, we use a root of zero and then send
       the result to the root.  To see this, note that the ordering is
       mask = 1: (ab)(cd)(ef)(gh)            (odds send to evens)
       mask = 2: ((ab)(cd))((ef)(gh))        (3,6 send to 0,4)
       mask = 4: (((ab)(cd))((ef)(gh)))      (4 sends to 0)
       
       Comments on buffering.  
       If the datatype is not contiguous, we still need to pass contiguous 
       data to the user routine.  
       In this case, we should make a copy of the data in some format, 
       and send/operate on that.
       
       In general, we can't use MPI_PACK, because the alignment of that
       is rather vague, and the data may not be re-usable.  What we actually
       need is a "squeeze" operation that removes the skips.
    */
        mask    = 0x1;
        if (is_commutative) 
            lroot   = root;
        else
            lroot   = 0;
        relrank = (rank - lroot + comm_size) % comm_size;
        
        while (/*(mask & relrank) == 0 && */mask < comm_size) {
            /* Receive */
            if ((mask & relrank) == 0) {
                source = (relrank | mask);
                if (source < comm_size) {
                    source = (source + lroot) % comm_size;
                    mpi_errno = MPIC_Recv (tmp_buf, count, datatype, source, 
                                           MPIR_REDUCE_TAG, comm, &status);
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

                    /* The sender is above us, so the received buffer must be
                       the second argument (in the noncommutative case). */
                    if (is_commutative) {
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                            count, datatype, uop );
                        }
                        else 
#endif
                            (*uop)(tmp_buf, recvbuf, &count, &datatype);
                    }
                    else {
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( recvbuf, tmp_buf,
                                                            count, datatype, uop );
                        }
                        else 
#endif
                            (*uop)(recvbuf, tmp_buf, &count, &datatype);
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
			if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                    }
                }
            }
            else {
                /* I've received all that I'm going to.  Send my result to 
                   my parent */
                source = ((relrank & (~ mask)) + lroot) % comm_size;
                mpi_errno  = MPIC_Send( recvbuf, count, datatype, 
                                        source, MPIR_REDUCE_TAG, comm );
 		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
                break;
            }
            mask <<= 1;
        }

        if (!is_commutative && (root != 0))
	{
            if (rank == 0)
	    {
                mpi_errno  = MPIC_Send( recvbuf, count, datatype, root, 
                                        MPIR_REDUCE_TAG, comm );
            }
            else if (rank == root)
	    {
                mpi_errno = MPIC_Recv ( recvbuf, count, datatype, 0, 
                                        MPIR_REDUCE_TAG, comm, &status);
            }
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }
    }
        
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    /* --BEGIN ERROR HANDLING-- */
    if (MPIU_THREADPRIV_FIELD(op_errno)) {
	mpi_errno = MPIU_THREADPRIV_FIELD(op_errno);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    return (mpi_errno);

  fn_fail:
    goto fn_exit;
}
/* end:nested */

/* begin:nested */
/* Needed in intercommunicator allreduce */
int MPIR_Reduce_inter ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    int root, 
    MPID_Comm *comm_ptr )
{
/*  Intercommunicator reduce.
    Remote group does a local intracommunicator
    reduce to rank 0. Rank 0 then sends data to root.

    Cost: (lgp+1).alpha + n.(lgp+1).beta
*/

    static const char FCNAME[] = "MPIR_Reduce_inter";
    int rank, mpi_errno;
    MPI_Status status;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf=NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;
    MPIU_THREADPRIV_DECL;
    MPIU_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    
    comm = comm_ptr->handle;

    if (root == MPI_ROOT) {
            /* root receives data from rank 0 on remote group */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
        mpi_errno = MPIC_Recv(recvbuf, count, datatype, 0,
                              MPIR_REDUCE_TAG, comm, &status);
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr ); 
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
    else {
        /* remote group. Rank 0 allocates temporary buffer, does
           local intracommunicator reduce, and then sends the data
           to root. */
        
        rank = comm_ptr->rank;
        
        if (rank == 0) {
            mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                                  &true_extent);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

            MPID_Datatype_get_extent_macro(datatype, extent);
	    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPIR_Setup_intercomm_localcomm( comm_ptr );

        newcomm_ptr = comm_ptr->local_comm;
        
        /* now do a local reduce on this intracommunicator */
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype,
                                op, 0, newcomm_ptr);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        if (rank == 0)
	{
            MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
            mpi_errno = MPIC_Send(tmp_buf, count, datatype, root,
                                  MPIR_REDUCE_TAG, comm); 
            MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIR_Nest_decr();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce

/*@

MPI_Reduce - Reduces values on all processes to a single value

Input Parameters:
+ sendbuf - address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - reduce operation (handle) 
. root - rank of root process (integer) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - address of receive buffer (choice, 
 significant only at 'root') 

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_BUFFER_ALIAS

@*/
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	       MPI_Op op, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Reduce";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("coll");
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_REDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;
            MPID_Op *op_ptr = NULL;
            int rank;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                MPIR_ERRTEST_COUNT(count, mpi_errno);
                MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(datatype, datatype_ptr);
                    MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                    MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                }

                if (sendbuf != MPI_IN_PLACE)
                    MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);
                }
                else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            }

	    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(datatype, datatype_ptr);
                        MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);
                }
                
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(datatype, datatype_ptr);
                        MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);
                }
            }

	    MPIR_ERRTEST_OP(op, mpi_errno);

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_Op_check_dtype_table[op%16 - 1] )(datatype); 
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }

	MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;

        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Reduce != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Reduce(sendbuf, recvbuf, count,
                                               datatype, op, root, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
            /* intracommunicator */
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype,
                                    op, root, comm_ptr); 
	}
        else {
            /* intercommunicator */
            mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype,
	      op, root, comm_ptr); 
        }
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE);
    MPIU_THREAD_SINGLE_CS_EXIT("coll");
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_reduce", "**mpi_reduce %p %p %d %D %O %d %C", sendbuf, recvbuf, 
				     count, datatype, op, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
