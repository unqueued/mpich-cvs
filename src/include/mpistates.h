/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPISTATES_H
#define MPISTATES_H

/* Insert all the states to be logged here */

/* Note to package writers:
 * If you insert your timer states here, you
 * also need to insert a function call in 
 * MPIU_Timer_init to describe your states
 * to the logging library.
 * MPIU_Timer_init is probably found in:
 * mpich2/src/util/timing/timer.c
 */

enum MPID_TIMER_STATE
{
/* device states */
#ifdef MPID_STATE_LIST_MPID
MPID_STATE_LIST_MPID
#endif

/* bsocket states */
#ifdef MPID_STATE_LIST_BSOCKET
MPID_STATE_LIST_BSOCKET
#endif

/* pmi states */
#ifdef MPID_STATE_LIST_PMI
MPID_STATE_LIST_PMI
#endif

/* mpi states */
MPID_STATE_MPI_SEND,
MPID_STATE_MPI_RECV,
MPID_STATE_MPI_BSEND,
MPID_STATE_MPI_SSEND,
MPID_STATE_MPI_RSEND,
MPID_STATE_MPI_ISEND,
MPID_STATE_MPI_IBSEND,
MPID_STATE_MPI_ISSEND,
MPID_STATE_MPI_IRSEND,
MPID_STATE_MPI_IRECV,
MPID_STATE_MPI_WAIT,
MPID_STATE_MPI_TEST,
MPID_STATE_MPI_SENDRECV,
MPID_STATE_MPI_SENDRECV_REPLACE,
MPID_STATE_MPI_WAITANY,
MPID_STATE_MPI_TESTANY,
MPID_STATE_MPI_WAITALL,
MPID_STATE_MPI_TESTALL,
MPID_STATE_MPI_WAITSOME,
MPID_STATE_MPI_TESTSOME,
MPID_STATE_MPI_IPROBE,
MPID_STATE_MPI_PROBE,
MPID_STATE_MPI_CANCEL,
MPID_STATE_MPI_TEST_CANCELLED,
MPID_STATE_MPI_SEND_INIT,
MPID_STATE_MPI_BSEND_INIT,
MPID_STATE_MPI_SSEND_INIT,
MPID_STATE_MPI_RSEND_INIT,
MPID_STATE_MPI_RECV_INIT,
MPID_STATE_MPI_START,
MPID_STATE_MPI_STARTALL,
MPID_STATE_MPI_BUFFER_ATTACH,
MPID_STATE_MPI_BUFFER_DETACH,
MPID_STATE_MPI_REQUEST_FREE,

MPID_STATE_MPI_GET_COUNT,
MPID_STATE_MPI_GET_ELEMENTS,

MPID_STATE_MPI_TYPE_CONTIGUOUS,
MPID_STATE_MPI_TYPE_VECTOR,
MPID_STATE_MPI_TYPE_HVECTOR,
MPID_STATE_MPI_TYPE_INDEXED,
MPID_STATE_MPI_TYPE_HINDEXED,
MPID_STATE_MPI_TYPE_STRUCT,
MPID_STATE_MPI_TYPE_EXTENT,
MPID_STATE_MPI_TYPE_SIZE,
MPID_STATE_MPI_TYPE_COUNT,
MPID_STATE_MPI_TYPE_LB,
MPID_STATE_MPI_TYPE_UB,
MPID_STATE_MPI_TYPE_COMMIT,
MPID_STATE_MPI_TYPE_FREE,
MPID_STATE_MPI_TYPE_CREATE_DARRAY,
MPID_STATE_MPI_TYPE_CREATE_HINDEXED,
MPID_STATE_MPI_TYPE_CREATE_HVECTOR,
MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK,
MPID_STATE_MPI_TYPE_CREATE_RESIZED,
MPID_STATE_MPI_TYPE_CREATE_STRUCT,
MPID_STATE_MPI_TYPE_CREATE_SUBARRAY,
MPID_STATE_MPI_TYPE_GET_EXTENT,
MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT,
MPID_STATE_MPI_ADDRESS,

MPID_STATE_MPI_PACK,
MPID_STATE_MPI_UNPACK,
MPID_STATE_MPI_PACK_SIZE,

MPID_STATE_MPI_BARRIER,
MPID_STATE_MPI_BCAST,
MPID_STATE_MPI_ALLREDUCE,
MPID_STATE_MPI_GATHER, 
MPID_STATE_MPI_GATHERV, 
MPID_STATE_MPI_SCATTER,
MPID_STATE_MPI_SCATTERV,
MPID_STATE_MPI_ALLGATHER,
MPID_STATE_MPI_ALLGATHERV,
MPID_STATE_MPI_ALLTOALL,
MPID_STATE_MPI_ALLTOALLV,
MPID_STATE_MPI_ALLTOALLW,
MPID_STATE_MPI_EXSCAN,
MPID_STATE_MPI_REDUCE,
MPID_STATE_MPI_REDUCE_SCATTER,
MPID_STATE_MPI_SCAN,
MPID_STATE_MPI_OP_CREATE,
MPID_STATE_MPI_OP_FREE,

MPID_STATE_MPI_GROUP_SIZE,
MPID_STATE_MPI_GROUP_RANK,
MPID_STATE_MPI_GROUP_TRANSLATE_RANKS ,
MPID_STATE_MPI_GROUP_COMPARE,
MPID_STATE_MPI_COMM_GROUP,
MPID_STATE_MPI_GROUP_UNION,
MPID_STATE_MPI_GROUP_INTERSECTION,
MPID_STATE_MPI_GROUP_DIFFERENCE,
MPID_STATE_MPI_GROUP_INCL,
MPID_STATE_MPI_GROUP_EXCL,
MPID_STATE_MPI_GROUP_RANGE_INCL,
MPID_STATE_MPI_GROUP_RANGE_EXCL,
MPID_STATE_MPI_GROUP_FREE,

MPID_STATE_MPI_COMM_DUP,
MPID_STATE_MPI_COMM_SIZE,
MPID_STATE_MPI_COMM_RANK,
MPID_STATE_MPI_COMM_COMPARE,
MPID_STATE_MPI_COMM_CREATE,
MPID_STATE_MPI_COMM_SPLIT,
MPID_STATE_MPI_COMM_FREE,
MPID_STATE_MPI_COMM_TEST_INTER,
MPID_STATE_MPI_COMM_REMOTE_SIZE,
MPID_STATE_MPI_COMM_REMOTE_GROUP,

MPID_STATE_MPI_INTERCOMM_CREATE,
MPID_STATE_MPI_INTERCOMM_MERGE,

MPID_STATE_MPI_KEYVAL_CREATE,
MPID_STATE_MPI_KEYVAL_FREE,
MPID_STATE_MPI_ATTR_PUT,
MPID_STATE_MPI_ATTR_GET,
MPID_STATE_MPI_ATTR_DELETE,

MPID_STATE_MPI_TOPO_TEST,
MPID_STATE_MPI_CART_CREATE,
MPID_STATE_MPI_DIMS_CREATE,
MPID_STATE_MPI_GRAPH_CREATE,
MPID_STATE_MPI_GRAPHDIMS_GET,
MPID_STATE_MPI_GRAPH_GET,
MPID_STATE_MPI_CARTDIM_GET,
MPID_STATE_MPI_CART_GET,
MPID_STATE_MPI_CART_RANK,
MPID_STATE_MPI_CART_COORDS,
MPID_STATE_MPI_GRAPH_NEIGHBORS_COUNT,
MPID_STATE_MPI_GRAPH_NEIGHBORS,
MPID_STATE_MPI_CART_SHIFT,
MPID_STATE_MPI_CART_SUB,
MPID_STATE_MPI_CART_MAP,
MPID_STATE_MPI_GRAPH_MAP,

MPID_STATE_MPI_GET_PROCESSOR_NAME,
MPID_STATE_MPI_GET_VERSION,

MPID_STATE_MPI_ERRHANDLER_CREATE,
MPID_STATE_MPI_ERRHANDLER_SET,
MPID_STATE_MPI_ERRHANDLER_GET,
MPID_STATE_MPI_ERRHANDLER_FREE,
MPID_STATE_MPI_ERROR_STRING,
MPID_STATE_MPI_ERROR_CLASS,

MPID_STATE_MPI_WTIME,
MPID_STATE_MPI_WTICK,

MPID_STATE_MPI_INIT,
MPID_STATE_MPI_FINALIZE,
MPID_STATE_MPI_INITIALIZED,
MPID_STATE_MPI_ABORT,

MPID_STATE_MPI_PCONTROL,

MPID_STATE_MPI_STATUS_F2C,
MPID_STATE_MPI_STATUS_C2F,

MPID_STATE_MPI_FINALIZED,
MPID_STATE_MPI_IS_THREAD_MAIN,
MPID_STATE_MPI_QUERY_THREAD,

MPID_STATE_MPI_STATUS_SET_CANCELLED,
MPID_STATE_MPI_STATUS_SET_ELEMENTS,

MPID_STATE_MPI_CLOSE_PORT,
MPID_STATE_MPI_COMM_ACCEPT,
MPID_STATE_MPI_COMM_CONNECT,
MPID_STATE_MPI_COMM_DISCONNECT,
MPID_STATE_MPI_COMM_GET_PARENT,
MPID_STATE_MPI_COMM_JOIN,
MPID_STATE_MPI_COMM_SPAWN,
MPID_STATE_MPI_COMM_SPAWN_MULTIPLE, 

MPID_STATE_MPI_LOOKUP_NAME,
MPID_STATE_MPI_OPEN_PORT,
MPID_STATE_MPI_PUBLISH_NAME,
MPID_STATE_MPI_UNPUBLISH_NAME,

MPID_STATE_MPI_ACCUMULATE,
MPID_STATE_MPI_GET,
MPID_STATE_MPI_PUT,
MPID_STATE_MPI_WIN_COMPLETE,
MPID_STATE_MPI_WIN_CREATE,
MPID_STATE_MPI_WIN_FENCE,
MPID_STATE_MPI_WIN_FREE,
MPID_STATE_MPI_WIN_GET_GROUP,
MPID_STATE_MPI_WIN_LOCK,
MPID_STATE_MPI_WIN_POST,
MPID_STATE_MPI_WIN_START,
MPID_STATE_MPI_WIN_TEST,
MPID_STATE_MPI_WIN_UNLOCK,
MPID_STATE_MPI_WIN_WAIT,


MPID_STATE_MPI_ADD_ERROR_CLASS,
MPID_STATE_MPI_ADD_ERROR_CODE,
MPID_STATE_MPI_ADD_ERROR_STRING,
MPID_STATE_MPI_COMM_CALL_ERRHANDLER,

MPID_STATE_MPI_COMM_CREATE_KEYVAL,
MPID_STATE_MPI_COMM_DELETE_ATTR,
MPID_STATE_MPI_COMM_FREE_KEYVAL,
MPID_STATE_MPI_COMM_GET_ATTR,
MPID_STATE_MPI_COMM_GET_NAME,
MPID_STATE_MPI_COMM_SET_ATTR,
MPID_STATE_MPI_COMM_SET_NAME,
MPID_STATE_MPI_FILE_CALL_ERRHANDLER,
MPID_STATE_MPI_GREQUEST_COMPLETE,
MPID_STATE_MPI_GREQUEST_START,
MPID_STATE_MPI_INIT_THREAD,

MPID_STATE_MPI_TYPE_GET_ENVELOPE, 
MPID_STATE_MPI_TYPE_GET_CONTENTS,
MPID_STATE_MPI_TYPE_CREATE_KEYVAL,
MPID_STATE_MPI_TYPE_DELETE_ATTR,
MPID_STATE_MPI_TYPE_DUP,
MPID_STATE_MPI_TYPE_FREE_KEYVAL,
MPID_STATE_MPI_TYPE_GET_ATTR,
MPID_STATE_MPI_TYPE_GET_NAME,
MPID_STATE_MPI_TYPE_SET_ATTR,
MPID_STATE_MPI_TYPE_SET_NAME,
MPID_STATE_MPI_WIN_CALL_ERRHANDLER,
MPID_STATE_MPI_WIN_CREATE_KEYVAL,
MPID_STATE_MPI_WIN_DELETE_ATTR,
MPID_STATE_MPI_WIN_FREE_KEYVAL,
MPID_STATE_MPI_WIN_GET_ATTR,
MPID_STATE_MPI_WIN_GET_NAME,
MPID_STATE_MPI_WIN_SET_ATTR,
MPID_STATE_MPI_WIN_SET_NAME,
MPID_STATE_MPI_ALLOC_MEM,
MPID_STATE_MPI_COMM_CREATE_ERRHANDLER,
MPID_STATE_MPI_COMM_GET_ERRHANDLER,
MPID_STATE_MPI_COMM_SET_ERRHANDLER,
MPID_STATE_MPI_FILE_CREATE_ERRHANDLER,
MPID_STATE_MPI_FILE_GET_ERRHANDLER,
MPID_STATE_MPI_FILE_SET_ERRHANDLER,
MPID_STATE_MPI_FREE_MEM,

MPID_STATE_MPI_GET_ADDRESS,

/* Info */
MPID_STATE_MPI_INFO_CREATE,
MPID_STATE_MPI_INFO_DELETE,
MPID_STATE_MPI_INFO_DUP,
MPID_STATE_MPI_INFO_FREE,
MPID_STATE_MPI_INFO_GET,
MPID_STATE_MPI_INFO_GET_NKEYS,
MPID_STATE_MPI_INFO_GET_NTHKEY,
MPID_STATE_MPI_INFO_GET_VALUELEN,
MPID_STATE_MPI_INFO_SET,

MPID_STATE_MPI_PACK_EXTERNAL, 
MPID_STATE_MPI_PACK_EXTERNAL_SIZE, 
MPID_STATE_MPI_REQUEST_GET_STATUS,
MPID_STATE_MPI_UNPACK_EXTERNAL, 
MPID_STATE_MPI_WIN_CREATE_ERRHANDLER,
MPID_STATE_MPI_WIN_GET_ERRHANDLER,
MPID_STATE_MPI_WIN_SET_ERRHANDLER,

/* internal mpi states */
MPID_STATE_MPIC_SEND,
MPID_STATE_MPIC_RECV,
MPID_STATE_MPIC_SENDRECV,

MPID_STATE_MPIR_WAIT,

MPID_NUM_TIMER_STATES
};

#ifdef FOO
/* 
   This file maps each MPI function name to a state def value for
   quick access.  For best logging performance, these should be 
   sorted by expected usage, so that the common routines have low
   numbers.
   
   0 is reserved for "unassigned"
*/
#define MPID_STATE_MPI_SEND 1
#define MPID_STATE_MPI_RECV 2
#define MPID_STATE_MPI_BSEND 3
#define MPID_STATE_MPI_SSEND 4
#define MPID_STATE_MPI_RSEND 5
#define MPID_STATE_MPI_ISEND 6
#define MPID_STATE_MPI_IBSEND 7
#define MPID_STATE_MPI_ISSEND 8
#define MPID_STATE_MPI_IRSEND 9
#define MPID_STATE_MPI_IRECV 10
#define MPID_STATE_MPI_WAIT 11
#define MPID_STATE_MPI_TEST 12
#define MPID_STATE_MPI_SENDRECV 13
#define MPID_STATE_MPI_SENDRECV_REPLACE 14
#define MPID_STATE_MPI_WAITANY 15
#define MPID_STATE_MPI_TESTANY 16
#define MPID_STATE_MPI_WAITALL 17
#define MPID_STATE_MPI_TESTALL 18
#define MPID_STATE_MPI_WAITSOME 19
#define MPID_STATE_MPI_TESTSOME 20
#define MPID_STATE_MPI_IPROBE 21
#define MPID_STATE_MPI_PROBE 22
#define MPID_STATE_MPI_CANCEL 23
#define MPID_STATE_MPI_TEST_CANCELLED 24
#define MPID_STATE_MPI_SEND_INIT 25
#define MPID_STATE_MPI_BSEND_INIT 26
#define MPID_STATE_MPI_SSEND_INIT 27
#define MPID_STATE_MPI_RSEND_INIT 28
#define MPID_STATE_MPI_RECV_INIT 29
#define MPID_STATE_MPI_START 30
#define MPID_STATE_MPI_STARTALL 31
#define MPID_STATE_MPI_BUFFER_ATTACH 32
#define MPID_STATE_MPI_BUFFER_DETACH 33
#define MPID_STATE_MPI_REQUEST_FREE 34

#define MPID_STATE_MPI_GET_COUNT 0
#define MPID_STATE_MPI_GET_ELEMENTS 0

#define MPID_STATE_MPI_TYPE_CONTIGUOUS 0
#define MPID_STATE_MPI_TYPE_VECTOR 0
#define MPID_STATE_MPI_TYPE_HVECTOR 0
#define MPID_STATE_MPI_TYPE_INDEXED 0
#define MPID_STATE_MPI_TYPE_HINDEXED 0
#define MPID_STATE_MPI_TYPE_STRUCT 0
#define MPID_STATE_MPI_TYPE_EXTENT 0
#define MPID_STATE_MPI_TYPE_SIZE 0
#define MPID_STATE_MPI_TYPE_COUNT 0
#define MPID_STATE_MPI_TYPE_LB 0
#define MPID_STATE_MPI_TYPE_UB 0
#define MPID_STATE_MPI_TYPE_COMMIT 0
#define MPID_STATE_MPI_TYPE_FREE 0
#define MPID_STATE_MPI_TYPE_CREATE_DARRAY 0
#define MPID_STATE_MPI_TYPE_CREATE_HINDEXED 0
#define MPID_STATE_MPI_TYPE_CREATE_HVECTOR 0
#define MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK 0
#define MPID_STATE_MPI_TYPE_CREATE_RESIZED 0
#define MPID_STATE_MPI_TYPE_CREATE_STRUCT 0
#define MPID_STATE_MPI_TYPE_CREATE_SUBARRAY 0
#define MPID_STATE_MPI_TYPE_GET_EXTENT 0
#define MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT 0
#define MPID_STATE_MPI_ADDRESS 0

#define MPID_STATE_MPI_PACK 0
#define MPID_STATE_MPI_UNPACK 0
#define MPID_STATE_MPI_PACK_SIZE 0

#define MPID_STATE_MPI_BARRIER 0
#define MPID_STATE_MPI_BCAST 0
#define MPID_STATE_MPI_ALLREDUCE 0
#define MPID_STATE_MPI_GATHER 0 
#define MPID_STATE_MPI_GATHERV 0 
#define MPID_STATE_MPI_SCATTER 0
#define MPID_STATE_MPI_SCATTERV 0
#define MPID_STATE_MPI_ALLGATHER 0
#define MPID_STATE_MPI_ALLGATHERV 0
#define MPID_STATE_MPI_ALLTOALL 0
#define MPID_STATE_MPI_ALLTOALLV 0
#define MPID_STATE_MPI_ALLTOALLW 0
#define MPID_STATE_MPI_EXSCAN  0
#define MPID_STATE_MPI_REDUCE 0
#define MPID_STATE_MPI_REDUCE_SCATTER 0
#define MPID_STATE_MPI_SCAN 0
#define MPID_STATE_MPI_OP_CREATE 0
#define MPID_STATE_MPI_OP_FREE 0

#define MPID_STATE_MPI_GROUP_SIZE 0
#define MPID_STATE_MPI_GROUP_RANK 0
#define MPID_STATE_MPI_GROUP_TRANSLATE_RANKS  0
#define MPID_STATE_MPI_GROUP_COMPARE 0
#define MPID_STATE_MPI_COMM_GROUP 0
#define MPID_STATE_MPI_GROUP_UNION 0
#define MPID_STATE_MPI_GROUP_INTERSECTION 0
#define MPID_STATE_MPI_GROUP_DIFFERENCE 0
#define MPID_STATE_MPI_GROUP_INCL 0
#define MPID_STATE_MPI_GROUP_EXCL 0
#define MPID_STATE_MPI_GROUP_RANGE_INCL 0
#define MPID_STATE_MPI_GROUP_RANGE_EXCL 0
#define MPID_STATE_MPI_GROUP_FREE 0

#define MPID_STATE_MPI_COMM_DUP 0
#define MPID_STATE_MPI_COMM_SIZE 0
#define MPID_STATE_MPI_COMM_RANK 0
#define MPID_STATE_MPI_COMM_COMPARE 0
#define MPID_STATE_MPI_COMM_CREATE 0
#define MPID_STATE_MPI_COMM_SPLIT 0
#define MPID_STATE_MPI_COMM_FREE 0
#define MPID_STATE_MPI_COMM_TEST_INTER 0
#define MPID_STATE_MPI_COMM_REMOTE_SIZE 0
#define MPID_STATE_MPI_COMM_REMOTE_GROUP 0

#define MPID_STATE_MPI_INTERCOMM_CREATE 0
#define MPID_STATE_MPI_INTERCOMM_MERGE 0

#define MPID_STATE_MPI_KEYVAL_CREATE 0
#define MPID_STATE_MPI_KEYVAL_FREE 0
#define MPID_STATE_MPI_ATTR_PUT 0
#define MPID_STATE_MPI_ATTR_GET 0
#define MPID_STATE_MPI_ATTR_DELETE 0

#define MPID_STATE_MPI_TOPO_TEST 0
#define MPID_STATE_MPI_CART_CREATE 0
#define MPID_STATE_MPI_DIMS_CREATE 0
#define MPID_STATE_MPI_GRAPH_CREATE 0
#define MPID_STATE_MPI_GRAPHDIMS_GET 0
#define MPID_STATE_MPI_GRAPH_GET 0
#define MPID_STATE_MPI_CARTDIM_GET 0
#define MPID_STATE_MPI_CART_GET 0
#define MPID_STATE_MPI_CART_RANK 0
#define MPID_STATE_MPI_CART_COORDS 0
#define MPID_STATE_MPI_GRAPH_NEIGHBORS_COUNT 0
#define MPID_STATE_MPI_GRAPH_NEIGHBORS 0
#define MPID_STATE_MPI_CART_SHIFT 0
#define MPID_STATE_MPI_CART_SUB 0
#define MPID_STATE_MPI_CART_MAP 0
#define MPID_STATE_MPI_GRAPH_MAP 0

#define MPID_STATE_MPI_GET_PROCESSOR_NAME 0
#define MPID_STATE_MPI_GET_VERSION 0

#define MPID_STATE_MPI_ERRHANDLER_CREATE 0
#define MPID_STATE_MPI_ERRHANDLER_SET 0
#define MPID_STATE_MPI_ERRHANDLER_GET 0
#define MPID_STATE_MPI_ERRHANDLER_FREE 0
#define MPID_STATE_MPI_ERROR_STRING 0
#define MPID_STATE_MPI_ERROR_CLASS 0

#define MPID_STATE_MPI_WTIME 0
#define MPID_STATE_MPI_WTICK 0

#define MPID_STATE_MPI_INIT 0
#define MPID_STATE_MPI_FINALIZE 0
#define MPID_STATE_MPI_INITIALIZED 0
#define MPID_STATE_MPI_ABORT 0
#define MPID_STATE_MPI_COMM_SET_NAME 0
#define MPID_STATE_MPI_COMM_GET_NAME 0

#define MPID_STATE_MPI_PCONTROL 0

#define MPID_STATE_MPI_STATUS_F2C 0
#define MPID_STATE_MPI_STATUS_C2F 0

#define MPID_STATE_MPI_FINALIZED 0
#define MPID_STATE_MPI_IS_THREAD_MAIN 0
#define MPID_STATE_MPI_QUERY_THREAD 0

#define MPID_STATE_MPI_STATUS_SET_CANCELLED 0
#define MPID_STATE_MPI_STATUS_SET_ELEMENTS 0

#define MPID_STATE_MPI_CLOSE_PORT 0
#define MPID_STATE_MPI_COMM_ACCEPT 0
#define MPID_STATE_MPI_COMM_CONNECT 0
#define MPID_STATE_MPI_COMM_DISCONNECT 0
#define MPID_STATE_MPI_COMM_GET_PARENT 0
#define MPID_STATE_MPI_COMM_JOIN 0
#define MPID_STATE_MPI_COMM_SPAWN 0
#define MPID_STATE_MPI_COMM_SPAWN_MULTIPLE 0 

#define MPID_STATE_MPI_LOOKUP_NAME 0
#define MPID_STATE_MPI_OPEN_PORT 0
#define MPID_STATE_MPI_PUBLISH_NAME 0
#define MPID_STATE_MPI_UNPUBLISH_NAME 0

#define MPID_STATE_MPI_ACCUMULATE 0
#define MPID_STATE_MPI_GET 0
#define MPID_STATE_MPI_PUT 0
#define MPID_STATE_MPI_WIN_COMPLETE 0
#define MPID_STATE_MPI_WIN_CREATE 0
#define MPID_STATE_MPI_WIN_FENCE 0
#define MPID_STATE_MPI_WIN_FREE 0
#define MPID_STATE_MPI_WIN_GET_GROUP 0
#define MPID_STATE_MPI_WIN_LOCK 0
#define MPID_STATE_MPI_WIN_POST 0
#define MPID_STATE_MPI_WIN_START 0
#define MPID_STATE_MPI_WIN_TEST 0
#define MPID_STATE_MPI_WIN_UNLOCK 0
#define MPID_STATE_MPI_WIN_WAIT 0


#define MPID_STATE_MPI_ADD_ERROR_CLASS 0
#define MPID_STATE_MPI_ADD_ERROR_CODE 0
#define MPID_STATE_MPI_ADD_ERROR_STRING 0
#define MPID_STATE_MPI_COMM_CALL_ERRHANDLER 0

#define MPID_STATE_MPI_COMM_CREATE_KEYVAL 0
#define MPID_STATE_MPI_COMM_DELETE_ATTR 0
#define MPID_STATE_MPI_COMM_FREE_KEYVAL 0
#define MPID_STATE_MPI_COMM_GET_ATTR 0
#define MPID_STATE_MPI_COMM_GET_NAME 0
#define MPID_STATE_MPI_COMM_SET_ATTR 0
#define MPID_STATE_MPI_COMM_SET_NAME 0
#define MPID_STATE_MPI_FILE_CALL_ERRHANDLER 0
#define MPID_STATE_MPI_GREQUEST_COMPLETE 0
#define MPID_STATE_MPI_GREQUEST_START 0
#define MPID_STATE_MPI_INIT_THREAD 0
#define MPID_STATE_MPI_IS_THREAD_MAIN 0
#define MPID_STATE_MPI_QUERY_THREAD 0

#define MPID_STATE_MPI_TYPE_GET_ENVELOPE 0 
#define MPID_STATE_MPI_TYPE_GET_CONTENTS 0
#define MPID_STATE_MPI_TYPE_CREATE_KEYVAL 0
#define MPID_STATE_MPI_TYPE_DELETE_ATTR 0
#define MPID_STATE_MPI_TYPE_DUP 0
#define MPID_STATE_MPI_TYPE_FREE_KEYVAL 0
#define MPID_STATE_MPI_TYPE_GET_ATTR 0
#define MPID_STATE_MPI_TYPE_GET_CONTENTS 0
#define MPID_STATE_MPI_TYPE_GET_ENVELOPE 0
#define MPID_STATE_MPI_TYPE_GET_NAME 0
#define MPID_STATE_MPI_TYPE_SET_ATTR 0
#define MPID_STATE_MPI_TYPE_SET_NAME 0
#define MPID_STATE_MPI_WIN_CALL_ERRHANDLER 0
#define MPID_STATE_MPI_WIN_CREATE_KEYVAL 0
#define MPID_STATE_MPI_WIN_DELETE_ATTR 0
#define MPID_STATE_MPI_WIN_FREE_KEYVAL 0
#define MPID_STATE_MPI_WIN_GET_ATTR 0
#define MPID_STATE_MPI_WIN_GET_NAME 0
#define MPID_STATE_MPI_WIN_SET_ATTR 0
#define MPID_STATE_MPI_WIN_SET_NAME 0
#define MPID_STATE_MPI_ALLOC_MEM 0
#define MPID_STATE_MPI_COMM_CREATE_ERRHANDLER 0
#define MPID_STATE_MPI_COMM_GET_ERRHANDLER 0
#define MPID_STATE_MPI_COMM_SET_ERRHANDLER 0
#define MPID_STATE_MPI_FILE_CREATE_ERRHANDLER 0
#define MPID_STATE_MPI_FILE_GET_ERRHANDLER 0
#define MPID_STATE_MPI_FILE_SET_ERRHANDLER 0
#define MPID_STATE_MPI_FINALIZED 0
#define MPID_STATE_MPI_FREE_MEM 0

#define MPID_STATE_MPI_GET_ADDRESS 0

/* Info */
#define MPID_STATE_MPI_INFO_CREATE 0
#define MPID_STATE_MPI_INFO_DELETE 0
#define MPID_STATE_MPI_INFO_DUP 0
#define MPID_STATE_MPI_INFO_FREE 0
#define MPID_STATE_MPI_INFO_GET 0
#define MPID_STATE_MPI_INFO_GET_NKEYS 0
#define MPID_STATE_MPI_INFO_GET_NTHKEY 0
#define MPID_STATE_MPI_INFO_GET_VALUELEN 0
#define MPID_STATE_MPI_INFO_SET 0

#define MPID_STATE_MPI_PACK_EXTERNAL 0 
#define MPID_STATE_MPI_PACK_EXTERNAL_SIZE 0 
#define MPID_STATE_MPI_REQUEST_GET_STATUS 0
#define MPID_STATE_MPI_UNPACK_EXTERNAL 0 
#define MPID_STATE_MPI_WIN_CREATE_ERRHANDLER 0
#define MPID_STATE_MPI_WIN_GET_ERRHANDLER 0
#define MPID_STATE_MPI_WIN_SET_ERRHANDLER 0

#endif

#endif
