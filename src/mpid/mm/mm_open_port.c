/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsocket.h"

int MM_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int bfd;
    int error;
    char host[40];
    int port;
    OpenPortNode_t *p;

    if (beasy_create(&bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
#ifdef HAVE_WINDOWS_H
	error = WSAGetLastError();
#else
	error = errno;
#endif
	printf("beasy_create failed, error %d\n", error);
	return error;
    }
    if (blisten(bfd, 5) == SOCKET_ERROR)
    {
#ifdef HAVE_WINDOWS_H
	error = WSAGetLastError();
#else
	error = errno;
#endif
	printf("blisten failed, error %d\n", error);
	return error;
    }
    beasy_get_sock_info(bfd, host, &port);
    beasy_get_ip_string(host);

    sprintf(port_name, "%s:%d", host, port);

    p = (OpenPortNode_t*)MPIU_Malloc(sizeof(OpenPortNode_t));
    p->bfd = bfd;
    strcpy(p->port_name, port_name);
    p->next = MPIR_Process.port_list;
    MPIR_Process.port_list = p;

    return 0;
}
