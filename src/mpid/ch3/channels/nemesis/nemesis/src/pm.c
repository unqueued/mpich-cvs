#include "pm.h"
#include "mpid_nem_debug.h"

#if 0
char MPID_nem_hostname[MAX_HOSTNAME_LEN] = "UNKNOWN";

#define ERROR(err...) do {					\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);			\
    printf ("%s ERROR. Exiting: %s\n", MPID_nem_hostname, MPID_nem_err_str);	\
    return -1;							\
} while (0)

#define safe_malloc(x) _safe_malloc(x, __FILE__, __LINE__)
static inline void *
_safe_malloc (size_t len, char* file, int line)
{
    void *p;

    p = MALLOC (len);
    if (p)
	return p;
    else
    {
	printf ("malloc failed at %s:%d\n", file, line);
	exit (-1);
	return 0;
    }
}

char *pmi_kvs_name;
char *pmi_key;
char *pmi_val;
int pmi_key_max_sz;
int pmi_val_max_sz;

int
pm_init (int *size, int *rank)
{
    int len;
    int ret;
    int spawned;

    gethostname (MPID_nem_hostname, MAX_HOSTNAME_LEN);
    MPID_nem_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    ret = PMI_Init (&spawned);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Init failed %d", ret);
    
    ret = PMI_Get_rank (rank);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_rank failed %d", ret);
    
    ret = PMI_Get_size (size);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_size failed %d", ret);

    printf_d ("  rank = %d\n  size = %d\n", *rank, *size);

    ret = PMI_KVS_Get_name_length_max(&len);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Get_name_length_max failed %d", ret);
    pmi_kvs_name = safe_malloc (len + 1);
    
    ret = PMI_KVS_Get_my_name(pmi_kvs_name, len);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Get_my_name failed %d", ret);

    ret = PMI_KVS_Get_key_length_max(&pmi_key_max_sz);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Get_key_length_max failed %d", ret);
    pmi_key = safe_malloc (pmi_key_max_sz);
    
    ret = PMI_KVS_Get_value_length_max(&pmi_val_max_sz);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Get_value_length_max failed %d", ret);
    pmi_val = safe_malloc (pmi_val_max_sz);

     return 0;
}

void
pm_finalize()
{
    PMI_Finalize();
}
#endif
