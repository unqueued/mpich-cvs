/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_get_business_card(char *value)
{
    sprintf(value, "%s:%d", TCP_Process.host, TCP_Process.port);

    return MPI_SUCCESS;
}
