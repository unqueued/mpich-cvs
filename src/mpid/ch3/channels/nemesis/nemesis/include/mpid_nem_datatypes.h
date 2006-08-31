/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_DATATYPES_H
#define MPID_NEM_DATATYPES_H

#include "mpid_nem_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sched.h>

#define MPID_NEM_OFFSETOF(struc, field) ((int)(&((struc *)0)->field))
#define MPID_NEM_CACHE_LINE_LEN 64
#define MPID_NEM_NUM_CELLS      64 

#ifndef MPID_NEM_NET_MODULE
#error MPID_NEM_NET_MODULE undefined
#endif
#ifndef MPID_NEM_DEFS_H
#error mpid_nem_defs.h must be included with this file
#endif

#if  !defined (MPID_NEM_NO_MODULE)
#error MPID_NEM_*_MODULEs are not defined!  Check for loop in include dependencies.
#endif

#if(MPID_NEM_NET_MODULE == MPID_NEM_ERROR_MODULE)
#error Error in definition of MPID_NEM_*_MODULE macros
#elif (MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)
#define MPID_NEM_CELL_LEN           (32*1024)
#elif (MPID_NEM_NET_MODULE == MPID_NEM_ELAN_MODULE)
#define MPID_NEM_CELL_LEN            (2*1024)
#else
#define MPID_NEM_CELL_LEN           (64*1024)
#endif 
#define MPID_NEM_CELL_PAYLOAD_LEN   (MPID_NEM_CELL_LEN - sizeof(void *))

#define MPID_NEM_CALC_CELL_LEN(cellp) (sizeof(void *) + MPID_NEM_MPICH2_HEAD_LEN + MPID_NEM_CELL_DLEN (cell))

#define MPID_NEM_ALIGNED(addr, bytes) ((((unsigned long)addr) & (((unsigned long)bytes)-1)) == 0)

#define MPID_NEM_PKT_UNKNOWN     0
#define MPID_NEM_PKT_CKPT        1
#define MPID_NEM_PKT_CKPT_REPLAY 2
#define MPID_NEM_PKT_MPICH2      3
#define MPID_NEM_PKT_MPICH2_HEAD 4

#define MPID_NEM_FBOX_SOURCE(cell) (MPID_nem_mem_region.local_procs[(cell)->pkt.mpich2.source])
#define MPID_NEM_CELL_SOURCE(cell) ((cell)->pkt.mpich2.source)
#define MPID_NEM_CELL_DEST(cell)   ((cell)->pkt.mpich2.dest)
#define MPID_NEM_CELL_DLEN(cell)   ((cell)->pkt.mpich2.datalen)
#define MPID_NEM_CELL_SEQN(cell)   ((cell)->pkt.mpich2.seqno)

#define MPID_NEM_MPICH2_HEAD_LEN sizeof(MPID_nem_pkt_header_t)
#define MPID_NEM_MPICH2_DATA_LEN (MPID_NEM_CELL_PAYLOAD_LEN - MPID_NEM_MPICH2_HEAD_LEN)

#define MPID_NEM_PKT_HEADER_FIELDS		\
    int source;					\
    int dest;					\
    int datalen;				\
    unsigned short seqno;                       \
    unsigned short type /* currently used only with checkpointing */
typedef struct MPID_nem_pkt_header
{
    MPID_NEM_PKT_HEADER_FIELDS;
} MPID_nem_pkt_header_t;

typedef struct MPID_nem_pkt_mpich2
{
    MPID_NEM_PKT_HEADER_FIELDS;
    char payload[MPID_NEM_MPICH2_DATA_LEN];
} MPID_nem_pkt_mpich2_t;

#ifdef ENABLED_CHECKPOINTING
/* checkpoint marker */
typedef struct MPID_nem_pkt_ckpt
{
    MPID_NEM_PKT_HEADER_FIELDS;
    unsigned short wave;
} MPID_nem_pkt_ckpt_t;
#endif

typedef union
{    
    MPID_nem_pkt_header_t      header;
    MPID_nem_pkt_mpich2_t      mpich2;
#ifdef ENABLED_CHECKPOINTING
    MPID_nem_pkt_ckpt_t        ckpt;
#endif
} MPID_nem_pkt_t;

/* Nemesis cells which are to be used in shared memory need to use
 * "relative pointers" because the absolute pointers to a cell from
 * different processes may be different.  Relative pointers are
 * offsets from the beginning of the mmapped region where they live.
 * We use different types for relative and absolute pointers to help
 * catch errors.  Use MPID_NEM_REL_TO_ABS and MPID_NEM_ABS_TO_REL to
 * convert between relative and absolute pointers. */

typedef struct MPID_nem_cell_rel_ptr
{
    char *p;
}
MPID_nem_cell_rel_ptr_t;

/* MPID_nem_cell and MPID_nem_abs_cell must be kept in sync so that we
 * can cast between them.  MPID_nem_abs_cell should only be used when
 * a cell is enqueued on a queue local to a single process (e.g., a
 * queue in a network module) where relative pointers are not
 * needed. */

typedef struct MPID_nem_cell
{
    MPID_nem_cell_rel_ptr_t next;
    MPID_nem_pkt_t pkt;
} MPID_nem_cell_t;
typedef volatile MPID_nem_cell_t *MPID_nem_cell_ptr_t;

typedef struct MPID_nem_abs_cell
{
    struct MPID_nem_abs_cell *next;
    volatile MPID_nem_pkt_t pkt;
} MPID_nem_abs_cell_t;
typedef MPID_nem_abs_cell_t *MPID_nem_abs_cell_ptr_t;

#define MPID_NEM_CELL_TO_PACKET(cellp) (&(cellp)->pkt)
#define MPID_NEM_PACKET_TO_CELL(packetp) \
    ((MPID_nem_cell_ptr_t) ((char*)(packetp) - (char *)MPID_NEM_CELL_TO_PACKET((MPID_nem_cell_ptr_t)0)))
#define MPID_NEM_MIN_PACKET_LEN (sizeof (MPID_nem_pkt_header_t))
#define MPID_NEM_MAX_PACKET_LEN (sizeof (MPID_nem_pkt_t))
#define MPID_NEM_PACKET_LEN(pkt) ((pkt)->mpich2.datalen + MPID_NEM_MPICH2_HEAD_LEN)

#define MPID_NEM_OPT_LOAD     16 
#define MPID_NEM_OPT_SIZE     ((sizeof(MPIDI_CH3_Pkt_t)) + (MPID_NEM_OPT_LOAD))
#define MPID_NEM_OPT_HEAD_LEN ((MPID_NEM_MPICH2_HEAD_LEN) + (MPID_NEM_OPT_SIZE))

#define MPID_NEM_PACKET_OPT_LEN(pkt) \
    (((pkt)->mpich2.datalen < MPID_NEM_OPT_SIZE) ? (MPID_NEM_OPT_HEAD_LEN) : (MPID_NEM_PACKET_LEN(pkt)))

#define MPID_NEM_PACKET_PAYLOAD(pkt) ((pkt)->mpich2.payload)

typedef struct MPID_nem_queue
{
    volatile MPID_nem_cell_rel_ptr_t head;
    volatile MPID_nem_cell_rel_ptr_t tail;
    char padding1[MPID_NEM_CACHE_LINE_LEN - 2 * sizeof(MPID_nem_cell_rel_ptr_t)];
    MPID_nem_cell_rel_ptr_t my_head;
    char padding2[MPID_NEM_CACHE_LINE_LEN - sizeof(MPID_nem_cell_rel_ptr_t)];
} MPID_nem_queue_t, *MPID_nem_queue_ptr_t;

/* Fast Boxes*/ 
typedef union
{
    volatile int value;
    char padding[MPID_NEM_CACHE_LINE_LEN];
}
MPID_nem_opt_volint_t;

typedef struct MPID_nem_fbox_common
{
    MPID_nem_opt_volint_t  flag;
} MPID_nem_fbox_common_t, *MPID_nem_fbox_common_ptr_t;

typedef struct MPID_nem_fbox_mpich2
{
    MPID_nem_opt_volint_t flag;
    MPID_nem_cell_t cell;
} MPID_nem_fbox_mpich2_t;

#define MPID_NEM_FBOX_DATALEN MPID_NEM_MPICH2_DATA_LEN

typedef union 
{
    MPID_nem_fbox_common_t common;
    MPID_nem_fbox_mpich2_t mpich2;
} MPID_nem_fastbox_t;


typedef struct MPID_nem_fbox_arrays
{
    MPID_nem_fastbox_t **in;
    MPID_nem_fastbox_t **out;
} MPID_nem_fbox_arrays_t;

#endif /* MPID_NEM_DATATYPES_H */
