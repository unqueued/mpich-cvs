#ifndef MM_TIMER_STATES_H
#define MM_TIMER_STATES_H

enum MM_TIMER_STATE
{
MM_OPEN_PORT_INDEX,
MM_CLOSE_PORT_INDEX,
MM_ACCEPT_INDEX,
MM_CONNECT_INDEX,
MM_SEND_INDEX,
MM_RECV_INDEX,
MM_CLOSE_INDEX,
MM_REQUEST_ALLOC_INDEX,
MM_REQUEST_FREE_INDEX,
MM_CAR_INIT_INDEX,
MM_CAR_FINALIZE_INDEX,
MM_CAR_ALLOC_INDEX,
MM_CAR_FREE_INDEX,
MM_VC_INIT_INDEX,
MM_VC_FINALIZE_INDEX,
MM_VC_FROM_COMMUNICATOR_INDEX,
MM_VC_FROM_CONTEXT_INDEX,
MM_VC_ALLOC_INDEX,
MM_VC_CONNECT_ALLOC_INDEX,
MM_VC_FREE_INDEX,
MM_CHOOSE_BUFFER_INDEX,
MM_RESET_CARS_INDEX,
MM_GET_BUFFERS_TMP_INDEX,
MM_RELEASE_BUFFERS_TMP_INDEX,
MM_GET_BUFFERS_VEC_INDEX,
VEC_BUFFER_INIT_INDEX,
TMP_BUFFER_INIT_INDEX,
MM_POST_RECV_INDEX,
MM_POST_SEND_INDEX,
MM_CQ_TEST_INDEX,
MM_CQ_WAIT_INDEX,
MM_CQ_ENQUEUE_INDEX,
MM_CREATE_POST_UNEX_INDEX,
MM_POST_UNEX_RNDV_INDEX,
XFER_INIT_INDEX,
XFER_RECV_OP_INDEX,
XFER_RECV_MOP_OP_INDEX,
XFER_RECV_FORWARD_OP_INDEX,
XFER_RECV_MOP_FORWARD_OP_INDEX,
XFER_FORWARD_OP_INDEX,
XFER_SEND_OP_INDEX,
XFER_REPLICATE_OP_INDEX,
XFER_START_INDEX,
TCP_INIT_INDEX,
TCP_FINALIZE_INDEX,
TCP_ACCEPT_CONNECTION_INDEX,
TCP_GET_BUSINESS_CARD_INDEX,
TCP_CAN_CONNECT_INDEX,
TCP_POST_CONNECT_INDEX,
TCP_POST_READ_INDEX,
TCP_MERGE_WITH_UNEXPECTED_INDEX,
TCP_POST_WRITE_INDEX,
TCP_MAKE_PROGRESS_INDEX,
TCP_CAR_ENQUEUE_INDEX,
TCP_CAR_DEQUEUE_INDEX,
TCP_RESET_CAR_INDEX,
TCP_POST_READ_PKT_INDEX,
TCP_READ_INDEX,
TCP_WRITE_INDEX,
TCP_READ_SHM_INDEX,
TCP_READ_VIA_INDEX,
TCP_READ_VIA_RDMA_INDEX,
TCP_READ_VEC_INDEX,
TCP_READ_TMP_INDEX,
TCP_READ_CONNECTING_INDEX,
TCP_WRITE_SHM_INDEX,
TCP_WRITE_VIA_INDEX,
TCP_WRITE_VIA_RDMA_INDEX,
TCP_WRITE_VEC_INDEX,
TCP_WRITE_TMP_INDEX,
TCP_STUFF_VECTOR_SHM_INDEX,
TCP_STUFF_VECTOR_VIA_INDEX,
TCP_STUFF_VECTOR_VIA_RDMA_INDEX,
TCP_STUFF_VECTOR_VEC_INDEX,
TCP_STUFF_VECTOR_TMP_INDEX,
TCP_WRITE_AGGRESSIVE_INDEX,

MM_NUM_TIMER_STATES
};

#endif
