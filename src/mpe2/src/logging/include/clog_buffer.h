/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_BUFFER )
#define _CLOG_BUFFER

#include "clog_common.h"
#include "clog_preamble.h"
#include "clog_block.h"

/*
   CLOG_DEFAULT_BLOCK_SIZE * CLOG_DEFAULT_BUFFERED_BLOCKS = 16 MB,
   It is total memory buffer size,
   i.e. data size written to disk per flush.
                                                                                
   The values defined here will be used by default
   if the corresponding variables are not used.
#define CLOG_DEFAULT_BLOCK_SIZE       131072
#define CLOG_DEFAULT_BUFFERED_BLOCKS     128
*/
#define CLOG_DEFAULT_BLOCK_SIZE        65536
#define CLOG_DEFAULT_BUFFERED_BLOCKS     128

/* 
   for testing CLOG_status, one bit for initialized and one for on/off
   0 - data structures are initialized and logging is ON
   1 - data structures are initialized and logging is OFF
   2 - data structures are not initialized, logging ON or OFF; error
*/

#define CLOG_INIT_AND_ON                   0
#define CLOG_INIT_AND_OFF                  1
#define CLOG_UNINIT                        2

typedef struct {
    CLOG_Preamble_t  *preamble;
    CLOG_Block_t     *head_block;
    CLOG_Block_t     *curr_block;
    unsigned int      block_size;
    unsigned int      num_blocks;
    unsigned int      num_used_blocks;

    int               num_mpi_procs;
    int               local_mpi_rank;

    int               local_fd;
    char              local_filename[ CLOG_PATH_STRLEN ];
    off_t             timeshift_fptr;
    int               delete_localfile;

    int               status;
} CLOG_Buffer_t;

CLOG_Buffer_t* CLOG_Buffer_create( void );

void CLOG_Buffer_free( CLOG_Buffer_t **buffer_handle );

void CLOG_Buffer_env_init( CLOG_Buffer_t *buffer );

void CLOG_Buffer_init( CLOG_Buffer_t *buffer, const char *local_tmpfile_name );

void CLOG_Buffer_localIO_init4write( CLOG_Buffer_t *buffer );

void CLOG_Buffer_localIO_write( CLOG_Buffer_t *buffer );

void CLOG_Buffer_advance_block( CLOG_Buffer_t *buffer );

void CLOG_Buffer_localIO_flush( CLOG_Buffer_t *buffer );

off_t CLOG_Buffer_localIO_ftell( CLOG_Buffer_t *buffer );

void CLOG_Buffer_init_timeshift( CLOG_Buffer_t *buffer );

void CLOG_Buffer_set_timeshift( CLOG_Buffer_t *buffer,
                                CLOG_Time_t    new_timediff,
                                int            init_next_timeshift );

void CLOG_Buffer_localIO_reinit4read( CLOG_Buffer_t *buffer );

void CLOG_Buffer_localIO_read( CLOG_Buffer_t *buffer );

void CLOG_Buffer_localIO_finalize( CLOG_Buffer_t *buffer );



void CLOG_Buffer_save_endlog( CLOG_Buffer_t *buffer );

void CLOG_Buffer_save_endblock( CLOG_Buffer_t *buffer );

void CLOG_Buffer_save_header( CLOG_Buffer_t *buffer, int rectype );

void CLOG_Buffer_save_statedef( CLOG_Buffer_t *buffer,
                                int stateID, int startetype, int finaletype,
                                const char *color, const char *name,
                                const char *format );

void CLOG_Buffer_save_eventdef( CLOG_Buffer_t *buffer, int etype,
                                const char *color, const char *name,
                                const char *format );

void CLOG_Buffer_save_constdef( CLOG_Buffer_t *buffer,
                                int etype, int value, const char *name );

void CLOG_Buffer_save_bareevt( CLOG_Buffer_t *buffer, int etype );

void CLOG_Buffer_save_cargoevt( CLOG_Buffer_t *buffer,
                                int etype, const char *bytes );

void CLOG_Buffer_save_msgevt( CLOG_Buffer_t *buffer,
                              int etype, int tag, int partner,
                              int comm, int size );

void CLOG_Buffer_save_collevt( CLOG_Buffer_t *buffer,
                               int etype, int root, int size, int comm );

void CLOG_Buffer_save_commevt( CLOG_Buffer_t *buffer,
                               int etype, int parent, int newcomm );

void CLOG_Buffer_save_srcloc( CLOG_Buffer_t *buffer,
                              int srcloc, int lineno, const char *filename );

void CLOG_Buffer_save_timeshift( CLOG_Buffer_t *buffer,
                                 CLOG_Time_t    timeshift );

#endif /* of _CLOG_BUFFER */
