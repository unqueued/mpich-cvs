/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "clog_record.h"
#include "clog_util.h"


/*@
     CLOG_Rec_print_rectype - print abbreviation shorthand for record type
     [ Support routines for CLOG_Rec_xxx_print() ]

. rectype - record type

@*/
void CLOG_Rec_print_rectype( int rectype, FILE *stream )
{
    switch (rectype) {
        case CLOG_REC_UNDEF:       fprintf( stream, "udef" ); break;
        case CLOG_REC_ENDLOG:      fprintf( stream, "elog" ); break;
        case CLOG_REC_ENDBLOCK:    fprintf( stream, "eblk" ); break;
        case CLOG_REC_STATEDEF:    fprintf( stream, "sdef" ); break;
        case CLOG_REC_EVENTDEF:    fprintf( stream, "edef" ); break;
        case CLOG_REC_CONSTDEF:    fprintf( stream, "cdef" ); break;
        case CLOG_REC_BAREEVT:     fprintf( stream, "bare" ); break;
        case CLOG_REC_CARGOEVT:    fprintf( stream, "cago" ); break;
        case CLOG_REC_MSGEVT:      fprintf( stream, "msg " ); break;
        case CLOG_REC_COLLEVT:     fprintf( stream, "coll" ); break;
        case CLOG_REC_COMMEVT:     fprintf( stream, "comm" ); break;
        case CLOG_REC_SRCLOC:      fprintf( stream, "loc " ); break;
        case CLOG_REC_TIMESHIFT:   fprintf( stream, "shft" ); break;
        default:                   fprintf( stream, "unknown(%d)", rectype);
    }
}

/*@
     CLOG_Rec_print_msgtype - print communication event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for pt2pt communication event

@*/
void CLOG_Rec_print_msgtype( int etype, FILE *stream )
{
    switch (etype) {
        case CLOG_EVT_SENDMSG: fprintf( stream, "send" ); break;
        case CLOG_EVT_RECVMSG: fprintf( stream, "recv" ); break;
            /* none predefined */
        default:               fprintf( stream, "unk(%d)", etype );
    }
}

/*@
     CLOG_Rec_print_commtype - print communicator creation event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for communicator creation event

@*/
void CLOG_Rec_print_commtype( int etype, FILE *stream )
{
    switch (etype) {
        case CLOG_COMM_INIT:   fprintf( stream, "init" ); break;
        case CLOG_COMM_DUP:    fprintf( stream, "dup " ); break;
        case CLOG_COMM_SPLIT:  fprintf( stream, "splt" ); break;
        case CLOG_COMM_CARTCR: fprintf( stream, "crtc" ); break;
        case CLOG_COMM_COMMCR: fprintf( stream, "cmmc" ); break;
        case CLOG_COMM_CFREE:  fprintf( stream, "free" ); break;
        default:               fprintf( stream, "unknown(%d)", etype );
    }
}

/*@
     CLOG_Rec_print_colltype - print collective event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for collective event

@*/
void CLOG_Rec_print_colltype( int etype, FILE *stream )
{
    switch (etype) {
        /* none predefined */
        default: fprintf( stream, "unk(%d)", etype );
    }
}




void CLOG_Rec_Header_swap_bytes( CLOG_Rec_Header_t *hdr )
{
    CLOG_Util_swap_bytes( &(hdr->timestamp), sizeof(CLOG_Time_t), 1 );
    CLOG_Util_swap_bytes( &(hdr->rectype), sizeof(int), 2 );
}

void CLOG_Rec_Header_print( CLOG_Rec_Header_t *hdr, FILE *stream )
{
    /* fprintf( stream, "ts=%f,%Lx ", hdr->timestamp, hdr->timestamp ); */
    fprintf( stream, "ts=%f ", hdr->timestamp );
    fprintf( stream, "type=" ); CLOG_Rec_print_rectype( hdr->rectype, stream );
    fprintf( stream, " proc=%d ", hdr->procID );
}

void CLOG_Rec_StateDef_swap_bytes( CLOG_Rec_StateDef_t *statedef )
{
    CLOG_Util_swap_bytes( &(statedef->stateID), sizeof(int), 3 );
    /* We do not adjust the 'pad' field */
    /* 'color' and 'name' fields are not adjusted */
}

void CLOG_Rec_StateDef_print( CLOG_Rec_StateDef_t *statedef, FILE *stream )
{
    fprintf( stream, "state=%d ", statedef->stateID );
    fprintf( stream, "s_et=%d ",  statedef->startetype );
    fprintf( stream, "e_et=%d ",  statedef->finaletype );
    fprintf( stream, "color=%s ", statedef->color );
    fprintf( stream, "name=%s ",  statedef->name );
    fprintf( stream, "fmt=%s\n",  statedef->format );
}

void CLOG_Rec_EventDef_swap_bytes( CLOG_Rec_EventDef_t *eventdef )
{
    CLOG_Util_swap_bytes( &(eventdef->etype), sizeof(int), 1 );
    /* 'pad' and 'name' are not adjusted */
}

void CLOG_Rec_EventDef_print( CLOG_Rec_EventDef_t *eventdef, FILE *stream )
{
    fprintf( stream, "et=%d ",    eventdef->etype );
    fprintf( stream, "color=%s ", eventdef->color );
    fprintf( stream, "name=%s ",  eventdef->name );
    fprintf( stream, "fmt=%s\n",  eventdef->format );
}

void CLOG_Rec_ConstDef_swap_bytes( CLOG_Rec_ConstDef_t *constdef )
{
    CLOG_Util_swap_bytes( &(constdef->etype), sizeof(int), 2 );
    /* 'name' are not adjusted */
}

void CLOG_Rec_ConstDef_print( CLOG_Rec_ConstDef_t *constdef, FILE *stream )
{
    fprintf( stream, "et=%d ",     constdef->etype );
    fprintf( stream, "name=%s ",   constdef->name );
    fprintf( stream, "value=%d\n", constdef->value );
}

void CLOG_Rec_BareEvt_swap_bytes( CLOG_Rec_BareEvt_t *bare )
{
    CLOG_Util_swap_bytes( &(bare->etype), sizeof(int), 1 );
}

void CLOG_Rec_BareEvt_print( CLOG_Rec_BareEvt_t *bare, FILE *stream )
{
    fprintf( stream, "et=%d\n", bare->etype );
}

void CLOG_Rec_CargoEvt_swap_bytes( CLOG_Rec_CargoEvt_t *cargo )
{
    CLOG_Util_swap_bytes( &(cargo->etype), sizeof(int), 1 );
    /* 'cargo->bytes[]' is adjusted by user */
}

void CLOG_Rec_CargoEvt_print( CLOG_Rec_CargoEvt_t *cargo, FILE *stream )
{
    fprintf( stream, "et=%d ",        cargo->etype );
    fprintf( stream, "bytes=%.32s\n", cargo->bytes );
}

void CLOG_Rec_MsgEvt_swap_bytes( CLOG_Rec_MsgEvt_t *msg )
{
    CLOG_Util_swap_bytes( &(msg->etype), sizeof(int), 5 );
    /* We do not adjust the 'pad' field */
}

void CLOG_Rec_MsgEvt_print( CLOG_Rec_MsgEvt_t *msg, FILE *stream )
{
    fprintf( stream,"et=" ); CLOG_Rec_print_msgtype( msg->etype, stream );
    fprintf( stream, " tg=%d ", msg->tag );
    fprintf( stream, "prt=%d ", msg->partner );
    fprintf( stream, "cm=%d ",  msg->comm );
    fprintf( stream, "sz=%d\n", msg->size );
}

void CLOG_Rec_CollEvt_swap_bytes( CLOG_Rec_CollEvt_t *coll )
{
    CLOG_Util_swap_bytes( &(coll->etype), sizeof(int), 4 );
}

void CLOG_Rec_CollEvt_print( CLOG_Rec_CollEvt_t *coll, FILE *stream )
{
    fprintf( stream, "et=" ); CLOG_Rec_print_colltype( coll->etype, stream );
    fprintf( stream, " root=%d ", coll->root);
    fprintf( stream, "cm=%d ",    coll->comm );
    fprintf( stream, "sz=%d\n",   coll->size );
}

void CLOG_Rec_CommEvt_swap_bytes( CLOG_Rec_CommEvt_t *comm )
{
    CLOG_Util_swap_bytes( &(comm->etype), sizeof(int), 3 );
    /* We do not adjust the 'pad' field */
}

void CLOG_Rec_CommEvt_print( CLOG_Rec_CommEvt_t *comm, FILE *stream )
{
    fprintf( stream, "et=" ); CLOG_Rec_print_commtype( comm->etype, stream );
    fprintf( stream, " pt=%d ",     comm->parent );
    fprintf( stream, "ncomm=%d\n", comm->newcomm );
}

void CLOG_Rec_Srcloc_swap_bytes( CLOG_Rec_Srcloc_t *src )
{
    CLOG_Util_swap_bytes( &(src->srcloc), sizeof(int), 2 );
    /* 'filename' is not adjusted */
}

void CLOG_Rec_Srcloc_print( CLOG_Rec_Srcloc_t *src, FILE *stream )
{
    fprintf( stream, "srcid=%d ", src->srcloc );
    fprintf( stream, "line=%d ",  src->lineno );
    fprintf( stream, "file=%s\n", src->filename );
}

void CLOG_Rec_Timeshift_swap_bytes( CLOG_Rec_Timeshift_t *tshift )
{
    CLOG_Util_swap_bytes( &(tshift->timeshift), sizeof(CLOG_Time_t), 1 );
}

void CLOG_Rec_Timeshift_print( CLOG_Rec_Timeshift_t *tshift, FILE *stream )
{
    fprintf( stream, "shift=%f\n", tshift->timeshift );
    /* fprintf( stream, "shift=%f,%Lx\n", tshift->timeshift, tshift->timeshift ); */
}

/*
   Assume readable(understandable) byte ordering before byteswapping
   i.e. byteswapping last.
*/
void CLOG_Rec_swap_bytes_last( CLOG_Rec_Header_t *hdr )
{
    int  rectype;
    /*
       Save hdr->rectype before byte swapping.
       After byteswapping hdr->rectype will not be understandable
    */
    rectype = hdr->rectype;
    CLOG_Rec_Header_swap_bytes( hdr );
    switch (rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_swap_bytes, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            break;
        case CLOG_REC_ENDBLOCK:
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_swap_bytes( (CLOG_Rec_StateDef_t *) hdr->rest );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_swap_bytes( (CLOG_Rec_EventDef_t *) hdr->rest );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_swap_bytes( (CLOG_Rec_ConstDef_t *) hdr->rest );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_swap_bytes( (CLOG_Rec_BareEvt_t *) hdr->rest );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_swap_bytes( (CLOG_Rec_CargoEvt_t *) hdr->rest );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_swap_bytes( (CLOG_Rec_MsgEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_swap_bytes( (CLOG_Rec_CollEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_swap_bytes( (CLOG_Rec_CommEvt_t *) hdr->rest );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_swap_bytes( (CLOG_Rec_Srcloc_t *) hdr->rest );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_swap_bytes( (CLOG_Rec_Timeshift_t *) hdr->rest );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_swap_bytes_last() - Warning!\n"
                             "\t""Unknown CLOG record type %d\n", rectype );
            fflush( stderr );
            break;
    }
}

/*
   Assume non-readable byte ordering before byteswapping
   i.e. byteswapping first
*/
void CLOG_Rec_swap_bytes_first( CLOG_Rec_Header_t *hdr )
{
    CLOG_Rec_Header_swap_bytes( hdr );
    /* After byteswapping, hdr->rectype is understandable */
    switch (hdr->rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_pre_swap_bytes, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            break;
        case CLOG_REC_ENDBLOCK:
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_swap_bytes( (CLOG_Rec_StateDef_t *) hdr->rest );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_swap_bytes( (CLOG_Rec_EventDef_t *) hdr->rest );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_swap_bytes( (CLOG_Rec_ConstDef_t *) hdr->rest );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_swap_bytes( (CLOG_Rec_BareEvt_t *) hdr->rest );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_swap_bytes( (CLOG_Rec_CargoEvt_t *) hdr->rest );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_swap_bytes( (CLOG_Rec_MsgEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_swap_bytes( (CLOG_Rec_CollEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_swap_bytes( (CLOG_Rec_CommEvt_t *) hdr->rest );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_swap_bytes( (CLOG_Rec_Srcloc_t *) hdr->rest );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_swap_bytes( (CLOG_Rec_Timeshift_t *) hdr->rest );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_swap_bytes_first() - Warning!\n"
                             "\t""Unknown CLOG record type %d\n",
                             hdr->rectype );
            fflush( stderr );
            break;
    }
}

void CLOG_Rec_print( CLOG_Rec_Header_t *hdr, FILE *stream )
{
    CLOG_Rec_Header_print( hdr, stream );
    switch (hdr->rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_print, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            fprintf( stream, "\n\n\n" );
            break;
        case CLOG_REC_ENDBLOCK:
            fprintf( stream, "\n\n" );
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_print( (CLOG_Rec_StateDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_print( (CLOG_Rec_EventDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_print( (CLOG_Rec_ConstDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_print( (CLOG_Rec_BareEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_print( (CLOG_Rec_CargoEvt_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_print( (CLOG_Rec_MsgEvt_t *) hdr->rest,
                                   stream );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_print( (CLOG_Rec_CollEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_print( (CLOG_Rec_CommEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_print( (CLOG_Rec_Srcloc_t *) hdr->rest,
                                   stream );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_print( (CLOG_Rec_Timeshift_t *) hdr->rest,
                                      stream );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_print() - \n"
                             "\t""Unrecognized CLOG record type %d\n",
                             hdr->rectype );
            fflush( stderr );
            break;
    }
    fflush( stream );
}

static int clog_reclens[ CLOG_REC_NUM ];

/*
    Pre-compute the record size or disk footprint of a complete record,
    i.e. CLOG_Rec_Header_t + CLOG_Rec_xxx_t, as given by CLOG_Rec_size().
*/
void CLOG_Rec_sizes_init( void )
{
    clog_reclens[ CLOG_REC_ENDLOG ]    = CLOG_RECLEN_HEADER;
    clog_reclens[ CLOG_REC_ENDBLOCK ]  = CLOG_RECLEN_HEADER;
    clog_reclens[ CLOG_REC_STATEDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_STATEDEF;
    clog_reclens[ CLOG_REC_EVENTDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_EVENTDEF;
    clog_reclens[ CLOG_REC_CONSTDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_CONSTDEF;
    clog_reclens[ CLOG_REC_BAREEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_BAREEVT;
    clog_reclens[ CLOG_REC_CARGOEVT ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_CARGOEVT;
    clog_reclens[ CLOG_REC_MSGEVT ]    = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_MSGEVT;
    clog_reclens[ CLOG_REC_COLLEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_COLLEVT;
    clog_reclens[ CLOG_REC_COMMEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_COMMEVT;
    clog_reclens[ CLOG_REC_SRCLOC ]    = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_SRCLOC;
    clog_reclens[ CLOG_REC_TIMESHIFT ] = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_TIMESHIFT;
}

/*
    CLOG_Rec_size() - returns complete record size on the disk,
                      ie. disk footprint.
*/
int CLOG_Rec_size( unsigned int rectype )
{
    if ( rectype < CLOG_REC_NUM )
        return clog_reclens[ rectype ];
    else {
        fprintf( stderr, __FILE__":CLOG_Rec_size() - Warning!"
                        "\t""Unknown record type %d\n", rectype );
        fflush( stderr );
        /*
           Assume it has at least a CLOG_Rec_Header_t,
           so length of CLOG_REC_ENDLOG
        */
        return clog_reclens[ CLOG_REC_ENDLOG ];
    }
}
