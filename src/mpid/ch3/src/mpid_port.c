/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int setupPortFunctions = 1;

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
static int MPIDI_Open_port(MPID_Info *, char *);

/* Define the functions that are used to implement the port operations.
   There is no "close port function in the default implementation 
   (hence the 0 in the portFns structure) */
static MPIDI_PortFns portFns = { MPIDI_Open_port, 0, 
				 MPIDI_Comm_accept, 
				 MPIDI_Comm_connect };
#else
static MPIDI_PortFns portFns = { 0, 0, 0, 0 };
#endif

/*@
   MPID_Open_port - Open an MPI Port

   Input Arguments:
.  MPI_Info info - info

   Output Arguments:
.  char *port_name - port name

   Notes:

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);

    /* Check to see if we need to setup channel-specific functions
       for handling the port operations */
    if (setupPortFunctions) {
	MPIU_CALL(MPIDI_CH3,PortFnsInit( &portFns ));
	setupPortFunctions = 0;
    }

    /* The default for this function is MPIDI_Open_port.
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Open_port.
       In addition, not all channels can implement this operation, so
       those channels will set the function pointer to NULL */
    if (portFns.OpenPort) {
	mpi_errno = portFns.OpenPort( info_ptr, port_name );
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl" );
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;
}

/*@
   MPID_Close_port - Close port

   Input Parameter:
.  port_name - Name of MPI port to close

   Notes:

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER

@*/
#undef FUNCNAME
#define FUNCNAME MPID_Close_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Close_port(const char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);

    /* Check to see if we need to setup channel-specific functions
       for handling the port operations */
    if (setupPortFunctions) {
	MPIU_CALL(MPIDI_CH3,PortFnsInit( &portFns ));
	setupPortFunctions = 0;
    }

    /* The default for this function is 0 (no function).
       A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Close_port */
    if (portFns.ClosePort) {
	mpi_errno = portFns.ClosePort( port_name );
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }

 fn_fail:	
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_accept(char * port_name, MPID_Info * info, int root, 
		     MPID_Comm * comm, MPID_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);

    /* Check to see if we need to setup channel-specific functions
       for handling the port operations */
    if (setupPortFunctions) {
	MPIU_CALL(MPIDI_CH3,PortFnsInit( &portFns ));
	setupPortFunctions = 0;
    }

    /* A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Comm_accept.
       If the function is null, we signal a not-implemented error */
    if (portFns.CommAccept) {
	mpi_errno = portFns.CommAccept( port_name, info, root, comm, 
					newcomm_ptr );
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl" );
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_connect(const char * port_name, MPID_Info * info, int root, 
		      MPID_Comm * comm, MPID_Comm ** newcomm_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);

    /* Check to see if we need to setup channel-specific functions
       for handling the port operations */
    if (setupPortFunctions) {
	MPIU_CALL(MPIDI_CH3,PortFnsInit( &portFns ));
	setupPortFunctions = 0;
    }

    /* A channel may define its own function and set it in the 
       init check above; such a function may be named MPIDI_CH3_Comm_connect.
       If the function is null, we signal a not-implemented error */
    if (portFns.CommConnect) {
	mpi_errno = portFns.CommConnect( port_name, info, root, comm, 
					 newcomm_ptr );
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else {
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**notimpl" );
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;
}

/* ------------------------------------------------------------------------- */
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

/*
 * Here are the routines that provide some of the default implementations
 * for the Port routines.
 *
 * MPIDI_Open_port - creates a port "name" that includes a tag value that
 * is used to separate different MPI Port values.  That tag value is
 * extracted with MPIDI_GetTagFromPort
 * MPIDI_GetTagFromPort - Routine to return the tag associated with a port.
 *
 * The port_name_tag is used in the connect and accept messages that 
 * are used in the connect/accept protocol that is implemented in the
 * ch3u_port.c file.
 */

#define MPIDI_CH3I_PORT_NAME_TAG_KEY "tag"

/*
 * MPIDI_Open_port()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    static int port_name_tag=0;   /* this tag is incremented and added to the 
				     business card, which is then returned 
				     as the port name */
    int myRank = MPIR_Process.comm_world->rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_OPEN_PORT);

    len = MPI_MAX_PORT_NAME;
    mpi_errno = MPIU_Str_add_int_arg(&port_name, &len, 
			MPIDI_CH3I_PORT_NAME_TAG_KEY, port_name_tag++);
    /* FIXME: MPIU_xxx routines should return regular mpi error codes */
    if (mpi_errno != MPIU_STR_SUCCESS) {
	mpi_errno = MPI_SUCCESS;
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**argstr_port_name_tag");
    }

    /* This works because Get_business_card uses the same MPIU_Str_xxx 
       functions as above to add the business card to the input string */
    /* FIXME: We should instead ask the mpid_pg routines to give us
       a connection string. There may need to be a separate step to 
       restrict us to a connection information that is only valid for
       connections between processes that are started separately (e.g.,
       may not use shared memory).  We may need a channel-specific 
       function to create an exportable connection string.  */
    mpi_errno = MPIU_CALL(MPIDI_CH3,Get_business_card(myRank, port_name, len));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_OPEN_PORT);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/*
 * The connect and accept routines use this routine to get the port tag
 * from the port name.
 */
int MPIDI_GetTagFromPort( const char *port_name, int *port_name_tag )
{
    int mpi_errno;

    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_NAME_TAG_KEY, 
				     port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPI_SUCCESS;
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**argstr_no_port_name_tag");
    }
    return mpi_errno;
}

#endif
