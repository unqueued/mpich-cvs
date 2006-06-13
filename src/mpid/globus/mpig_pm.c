/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if FALSE

/*
 * mpig_pm_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_init
int mpig_pm_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_init() */


/*
 * mpig_pm_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_finalize
int mpig_pm_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_finalize);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }/* --END ERROR HANDLING-- */
}
/* mpig_pm_finalize() */


/*
 * mpig_pm_abort()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_abort
int mpig_pm_abort(int exit_code)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_abort);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_abort);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_abort);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_abort() */


/*
 * mpig_pm_exchange_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_exchange_business_cards
int mpig_pm_exchange_business_cards(mpig_bc_t * const bc, mpig_bc_t ** const bcs_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_exchange_business_cards);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_exchange_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) bc));

    
    
  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
		       "exiting: bcs=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) *bcs_ptr, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_exchange_business_cards);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_exchange_business_cards() */


/*
 * mpig_pm_free_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_free_business_cards
int mpig_pm_free_business_cards(mpig_bc_t * const bcs)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_free_business_cards);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_free_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bcs=" MPIG_PTR_FMT, (MPIG_PTR_CAST) bcs));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_free_business_cards);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_free_business_cards() */


/*
 * mpig_pm_get_pg_size()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_size
int mpig_pm_get_pg_size(int * const pg_size)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_size);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_size);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: pg_size=%d, mpi_errno=" MPIG_ERRNO_FMT,
	(mpi_errno) ? -1 : *pg_size, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_size);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_size() */


/*
 * mpig_pm_get_pg_rank()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_rank
int mpig_pm_get_pg_rank(int * const pg_rank)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_rank);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_rank);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: pg_rank=%d, mpi_errno=" MPIG_ERRNO_FMT,
	(mpi_errno) ? -1 : *pg_rank, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_rank);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_rank() */


/*
 * mpig_pm_get_pg_id()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_id
int mpig_pm_get_pg_id(const char ** const pg_id)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_id);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_id);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: pg_id=%s, mpi_errno=" MPIG_ERRNO_FMT,
	(mpi_errno) ? "(null)" : *pg_id, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_id);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_id() */


/*
 * mpig_pm_get_app_num()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_app_num
int mpig_pm_get_app_num(const mpig_bc_t * const bc, int * const app_num_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_app_num);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_app_num);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));


    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: app_num=%d, mpi_errno=" MPIG_ERRNO_FMT,
	(mpi_errno) ? -1 : app_num, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_app_num);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_app_num() */

#endif /*FALSE */
