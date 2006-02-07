#include "tcp_module.h"
#include "tcp_module_impl.h"

int
tcp_module_finalize ()
{
    long int toto = 1000;
    
#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING SEND & RECV \n",MPID_nem_mem_region.rank);
#endif
    while( n_pending_send > 0 )
    {
	tcp_module_poll( 1 );
    }

#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING DONE  1\n",MPID_nem_mem_region.rank);
#endif
    while(--toto)
	tcp_module_poll( 1 );
#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING DONE  2\n",MPID_nem_mem_region.rank);
#endif

    return tcp_module_ckpt_shutdown ();    
}

int
tcp_module_ckpt_shutdown ()
{
    int         index;
    int         grank;
    
    /* close the sockets */
    for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	{
	    shutdown (nodes[grank].desc, SHUT_RDWR);
	    close (nodes[grank].desc);
	}
    }

#ifdef TRACE
    fprintf(stderr,"[%i] --- sockets closed .... \n",MPID_nem_mem_region.rank);
#endif

    return 0 ;
}

