/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   mm_connector_connect - connect

   Parameters:
+  struct MPIDI_VC *vc_ptr - virtual connection pointer

   Notes:
@*/
int mm_connector_connect(struct MPIDI_VC *vc_ptr)
{
    int rank;
    char *name;
    char key[100];
    char *value;
    int value_len;

    rank = vc_ptr->rank;
    name = vc_ptr->pmi_kvsname;

    value_len = PMI_KVS_Get_value_length_max();
    value = (char*)malloc(value_len);
    snprintf(key, 100, "businesscard:%d", rank);
    PMI_KVS_Get(name, key, value);

    /* figure out stuff */

    free(value);

    return 0;
}
