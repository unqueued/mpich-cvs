/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

/* MPI datatype debugging helper routines.
 * 
 * The one you want to call is:
 *   MPIDU_Datatype_debug(MPI_Datatype type, int array_ct)
 *
 * The "array_ct" value tells the call how many array values to print
 * for struct, indexed, and blockindexed types.
 *
 */


void MPIDI_Datatype_dot_printf(MPI_Datatype type, int depth, int header);
void MPIDI_Dataloop_dot_printf(MPID_Dataloop *loop_p, int depth, int header);
void MPIDI_Datatype_contents_printf(MPI_Datatype type, int depth, int acount);
static char *MPIDI_Datatype_depth_spacing(int depth);

/* note: this isn't really "error handling" per se, but leave these comments
 * because Bill uses them for coverage analysis.
 */

/* --BEGIN ERROR HANDLING-- */
void MPIDI_Datatype_dot_printf(MPI_Datatype type,
			       int depth,
			       int header)
{
    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	MPIU_dbg_printf("MPIDI_Datatype_dot_printf: type is a basic\n");
	return;
    }
    else {
	MPID_Datatype *dt_p;
	MPID_Dataloop *loop_p;

	MPID_Datatype_get_ptr(type, dt_p);
	loop_p = dt_p->dataloop;

	MPIDI_Dataloop_dot_printf(loop_p, depth, header);
	return;
    }
}

void MPIDI_Dataloop_dot_printf(MPID_Dataloop *loop_p,
			       int depth,
			       int header)
{
    int i;

    if (header) {
	MPIU_dbg_printf("digraph %d {   {\n", (int) loop_p);
    }

    switch (loop_p->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_CONTIG:
	    MPIU_dbg_printf("      dl%d [shape = record, label = \"contig |{ ct = %d; el_sz = %d; el_ext = %d }\"];\n",
			    depth,
			    (int) loop_p->loop_params.c_t.count,
			    (int) loop_p->el_size,
			    (int) loop_p->el_extent);
	    break;
	case DLOOP_KIND_VECTOR:
	    MPIU_dbg_printf("      dl%d [shape = record, label = \"vector |{ ct = %d; blk = %d; str = %d; el_sz = %d; el_ext = %d }\"];\n",
			    depth,
			    (int) loop_p->loop_params.v_t.count,
			    (int) loop_p->loop_params.v_t.blocksize,
			    (int) loop_p->loop_params.v_t.stride,
			    (int) loop_p->el_size,
			    (int) loop_p->el_extent);
	    break;
	case DLOOP_KIND_INDEXED:
	    MPIU_dbg_printf("      dl%d [shape = record, label = \"indexed |{ ct = %d; tot_blks = %d; regions = ",
			    depth,
			    (int) loop_p->loop_params.i_t.count,
			    (int) loop_p->loop_params.i_t.total_blocks);
	    
	    /* 3 picked as arbitrary cutoff */
	    for (i=0; i < 3 && i < loop_p->loop_params.i_t.count; i++) {
		if (i + 1 < loop_p->loop_params.i_t.count) {
		    /* more regions after this one */
		    MPIU_dbg_printf("(%d, %d), ",
				    (int) loop_p->loop_params.i_t.offset_array[i],
				    (int) loop_p->loop_params.i_t.blocksize_array[i]);
		}
		else {
		    MPIU_dbg_printf("(%d, %d); ",
				    (int) loop_p->loop_params.i_t.offset_array[i],
				    (int) loop_p->loop_params.i_t.blocksize_array[i]);
		}
	    }
	    if (i < loop_p->loop_params.i_t.count) {
		MPIU_dbg_printf("...; ");
	    }

	    MPIU_dbg_printf("el_sz = %d; el_ext = %d }\"];\n",
			    (int) loop_p->el_size,
			    (int) loop_p->el_extent);
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    MPIU_dbg_printf("      dl%d [shape = record, label = \"blockindexed |{ ct = %d; blk = %d; disps = ",
			    depth,
			    (int) loop_p->loop_params.bi_t.count,
			    (int) loop_p->loop_params.bi_t.blocksize);
	    
	    /* 3 picked as arbitrary cutoff */
	    for (i=0; i < 3 && i < loop_p->loop_params.bi_t.count; i++) {
		if (i + 1 < loop_p->loop_params.bi_t.count) {
		    /* more regions after this one */
		    MPIU_dbg_printf("%d, ",
				    (int) loop_p->loop_params.bi_t.offset_array[i]);
		}
		else {
		    MPIU_dbg_printf("%d; ",
				    (int) loop_p->loop_params.bi_t.offset_array[i]);
		}
	    }
	    if (i < loop_p->loop_params.bi_t.count) {
		MPIU_dbg_printf("...; ");
	    }

	    MPIU_dbg_printf("el_sz = %d; el_ext = %d }\"];\n",
			    (int) loop_p->el_size,
			    (int) loop_p->el_extent);
	    break;
	case DLOOP_KIND_STRUCT:
	    MPIU_dbg_printf("      dl%d [shape = record, label = \"struct | {ct = %d; blks = ",
			    depth,
			    (int) loop_p->loop_params.s_t.count);
	    for (i=0; i < 3 && i < loop_p->loop_params.s_t.count; i++) {
		if (i + 1 < loop_p->loop_params.s_t.count) {
		    MPIU_dbg_printf("%d, ",
				    (int) loop_p->loop_params.s_t.blocksize_array[i]);
		}
		else {
		    MPIU_dbg_printf("%d; ",
				    (int) loop_p->loop_params.s_t.blocksize_array[i]);
		}
	    }
	    if (i < loop_p->loop_params.s_t.count) {
		MPIU_dbg_printf("...; disps = ");
	    }
	    else {
		MPIU_dbg_printf("disps = ");
	    }

	    for (i=0; i < 3 && i < loop_p->loop_params.s_t.count; i++) {
		if (i + 1 < loop_p->loop_params.s_t.count) {
		    MPIU_dbg_printf("%d, ",
				    (int) loop_p->loop_params.s_t.offset_array[i]);
		}
		else {
		    MPIU_dbg_printf("%d; ",
				    (int) loop_p->loop_params.s_t.offset_array[i]);
		}
	    }
	    if (i < loop_p->loop_params.s_t.count) {
		MPIU_dbg_printf("... }\"];\n");
	    }
	    else {
		MPIU_dbg_printf("}\"];\n");
	    }
	    break;
	default:
	    MPIU_Assert(0);
    }

    if (!(loop_p->kind & DLOOP_FINAL_MASK)) {
	/* more loops to go; recurse */
	MPIU_dbg_printf("      dl%d -> dl%d;\n", depth, depth + 1);
	switch (loop_p->kind & DLOOP_KIND_MASK) {
	    case DLOOP_KIND_CONTIG:
		MPIDI_Dataloop_dot_printf(loop_p->loop_params.c_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_VECTOR:
		MPIDI_Dataloop_dot_printf(loop_p->loop_params.v_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_INDEXED:
		MPIDI_Dataloop_dot_printf(loop_p->loop_params.i_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_BLOCKINDEXED:
		MPIDI_Dataloop_dot_printf(loop_p->loop_params.bi_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_STRUCT:
		for (i=0; i < loop_p->loop_params.s_t.count; i++) {
		    MPIDI_Dataloop_dot_printf(loop_p->loop_params.s_t.dataloop_array[i],
					      depth + 1, 0);
		}
		break;
	    default:
		MPIU_dbg_printf("      < unsupported type >\n");
	}
    }


    if (header) {
	MPIU_dbg_printf("   }\n}\n");
    }
    return;
}

void MPIDI_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header)
{
    char *string;
    int size;
    MPI_Aint extent, true_lb, true_ub, lb, ub, sticky_lb, sticky_ub;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	string = MPIDU_Datatype_builtin_to_string(type);
	if (type == MPI_LB) sticky_lb = 1;
	else sticky_lb = 0;
	if (type == MPI_UB) sticky_ub = 1;
	else sticky_ub = 0;
    }
    else {
	MPID_Datatype *type_ptr;

	MPID_Datatype_get_ptr(type, type_ptr);
	string = MPIDU_Datatype_combiner_to_string(type_ptr->contents->combiner);
	sticky_lb = type_ptr->has_sticky_lb;
	sticky_ub = type_ptr->has_sticky_ub;
    }

    MPIR_Nest_incr();
    NMPI_Type_size(type, &size);
    NMPI_Type_get_true_extent(type, &true_lb, &extent);
    true_ub = extent + true_lb;
    NMPI_Type_get_extent(type, &lb, &extent);
    ub = extent + lb;
    MPIR_Nest_decr();

    if (header == 1) {
	/*               012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 */
	MPIU_dbg_printf("------------------------------------------------------------------------------------------------------------------------------------------\n");
	MPIU_dbg_printf("depth                   type         size       extent      true_lb      true_ub           lb(s)           ub(s)         disp       blklen\n");
	MPIU_dbg_printf("------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    MPIU_dbg_printf("%5d  %21s  %11d  %11d  %11d  %11d  %11d(%1d)  %11d(%1d)  %11d  %11d\n",
		    depth,
		    string,
		    (int) size,
		    (int) extent,
		    (int) true_lb,
		    (int) true_ub,
		    (int) lb,
		    (int) sticky_lb,
		    (int) ub,
		    (int) sticky_ub,
		    (int) displacement,
		    (int) blocklength);
    return;
}
/* --END ERROR HANDLING-- */

/* longest string is 21 characters */
char *MPIDU_Datatype_builtin_to_string(MPI_Datatype type)
{
    static char t_char[]             = "MPI_CHAR";
    static char t_uchar[]            = "MPI_UNSIGNED_CHAR";
    static char t_byte[]             = "MPI_BYTE";
    static char t_wchar_t[]          = "MPI_WCHAR";
    static char t_short[]            = "MPI_SHORT";
    static char t_ushort[]           = "MPI_UNSIGNED_SHORT";
    static char t_int[]              = "MPI_INT";
    static char t_uint[]             = "MPI_UNSIGNED";
    static char t_long[]             = "MPI_LONG";
    static char t_ulong[]            = "MPI_UNSIGNED_LONG";
    static char t_float[]            = "MPI_FLOAT";
    static char t_double[]           = "MPI_DOUBLE";
    static char t_longdouble[]       = "MPI_LONG_DOUBLE";
    static char t_longlongint[]      = "MPI_LONG_LONG_INT";
    static char t_longlong[]         = "MPI_LONG_LONG";
    static char t_ulonglong[]        = "MPI_UNSIGNED_LONG_LONG";
    static char t_schar[]            = "MPI_SIGNED_CHAR";

    static char t_packed[]           = "MPI_PACKED";
    static char t_lb[]               = "MPI_LB";
    static char t_ub[]               = "MPI_UB";

    static char t_floatint[]         = "MPI_FLOAT_INT";
    static char t_doubleint[]        = "MPI_DOUBLE_INT";
    static char t_longint[]          = "MPI_LONG_INT";
    static char t_shortint[]         = "MPI_SHORT_INT";
    static char t_2int[]             = "MPI_2INT";
    static char t_longdoubleint[]    = "MPI_LONG_DOUBLE_INT";

    static char t_complex[]          = "MPI_COMPLEX";
    static char t_doublecomplex[]    = "MPI_DOUBLE_COMPLEX";
    static char t_logical[]          = "MPI_LOGICAL";
    static char t_real[]             = "MPI_REAL";
    static char t_doubleprecision[]  = "MPI_DOUBLE_PRECISION";
    static char t_integer[]          = "MPI_INTEGER";
    static char t_2integer[]         = "MPI_2INTEGER";
    static char t_2complex[]         = "MPI_2COMPLEX";
    static char t_2doublecomplex[]   = "MPI_2DOUBLE_COMPLEX";
    static char t_2real[]            = "MPI_2REAL";
    static char t_2doubleprecision[] = "MPI_2DOUBLE_PRECISION";
    static char t_character[]        = "MPI_CHARACTER";

    if (type == MPI_CHAR)              return t_char;
    if (type == MPI_UNSIGNED_CHAR)     return t_uchar;
    if (type == MPI_SIGNED_CHAR)       return t_schar;
    if (type == MPI_BYTE)              return t_byte;
    if (type == MPI_WCHAR)             return t_wchar_t;
    if (type == MPI_SHORT)             return t_short;
    if (type == MPI_UNSIGNED_SHORT)    return t_ushort;
    if (type == MPI_INT)               return t_int;
    if (type == MPI_UNSIGNED)          return t_uint;
    if (type == MPI_LONG)              return t_long;
    if (type == MPI_UNSIGNED_LONG)     return t_ulong;
    if (type == MPI_FLOAT)             return t_float;
    if (type == MPI_DOUBLE)            return t_double;
    if (type == MPI_LONG_DOUBLE)       return t_longdouble;
    if (type == MPI_LONG_LONG_INT)     return t_longlongint;
    if (type == MPI_LONG_LONG)         return t_longlong;
    if (type == MPI_UNSIGNED_LONG_LONG) return t_ulonglong;
	
    if (type == MPI_PACKED)            return t_packed;
    if (type == MPI_LB)                return t_lb;
    if (type == MPI_UB)                return t_ub;
	
    if (type == MPI_FLOAT_INT)         return t_floatint;
    if (type == MPI_DOUBLE_INT)        return t_doubleint;
    if (type == MPI_LONG_INT)          return t_longint;
    if (type == MPI_SHORT_INT)         return t_shortint;
    if (type == MPI_2INT)              return t_2int;
    if (type == MPI_LONG_DOUBLE_INT)   return t_longdoubleint;
	
    if (type == MPI_COMPLEX)           return t_complex;
    if (type == MPI_DOUBLE_COMPLEX)    return t_doublecomplex;
    if (type == MPI_LOGICAL)           return t_logical;
    if (type == MPI_REAL)              return t_real;
    if (type == MPI_DOUBLE_PRECISION)  return t_doubleprecision;
    if (type == MPI_INTEGER)           return t_integer;
    if (type == MPI_2INTEGER)          return t_2integer;
    if (type == MPI_2COMPLEX)          return t_2complex;
    if (type == MPI_2DOUBLE_COMPLEX)   return t_2doublecomplex;
    if (type == MPI_2REAL)             return t_2real;
    if (type == MPI_2DOUBLE_PRECISION) return t_2doubleprecision;
    if (type == MPI_CHARACTER)         return t_character;
    
    return NULL;
}

/* MPIDU_Datatype_combiner_to_string(combiner)
 *
 * Converts a numeric combiner into a pointer to a string used for printing.
 *
 * longest string is 16 characters.
 */
char *MPIDU_Datatype_combiner_to_string(int combiner)
{
    static char c_named[]    = "named";
    static char c_contig[]   = "contig";
    static char c_vector[]   = "vector";
    static char c_hvector[]  = "hvector";
    static char c_indexed[]  = "indexed";
    static char c_hindexed[] = "hindexed";
    static char c_struct[]   = "struct";
    static char c_dup[]              = "dup";
    static char c_hvector_integer[]  = "hvector_integer";
    static char c_hindexed_integer[] = "hindexed_integer";
    static char c_indexed_block[]    = "indexed_block";
    static char c_struct_integer[]   = "struct_integer";
    static char c_subarray[]         = "subarray";
    static char c_darray[]           = "darray";
    static char c_f90_real[]         = "f90_real";
    static char c_f90_complex[]      = "f90_complex";
    static char c_f90_integer[]      = "f90_integer";
    static char c_resized[]          = "resized";

    if (combiner == MPI_COMBINER_NAMED)      return c_named;
    if (combiner == MPI_COMBINER_CONTIGUOUS) return c_contig;
    if (combiner == MPI_COMBINER_VECTOR)     return c_vector;
    if (combiner == MPI_COMBINER_HVECTOR)    return c_hvector;
    if (combiner == MPI_COMBINER_INDEXED)    return c_indexed;
    if (combiner == MPI_COMBINER_HINDEXED)   return c_hindexed;
    if (combiner == MPI_COMBINER_STRUCT)     return c_struct;
    if (combiner == MPI_COMBINER_DUP)              return c_dup;
    if (combiner == MPI_COMBINER_HVECTOR_INTEGER)  return c_hvector_integer;
    if (combiner == MPI_COMBINER_HINDEXED_INTEGER) return c_hindexed_integer;
    if (combiner == MPI_COMBINER_INDEXED_BLOCK)    return c_indexed_block;
    if (combiner == MPI_COMBINER_STRUCT_INTEGER)   return c_struct_integer;
    if (combiner == MPI_COMBINER_SUBARRAY)         return c_subarray;
    if (combiner == MPI_COMBINER_DARRAY)           return c_darray;
    if (combiner == MPI_COMBINER_F90_REAL)         return c_f90_real;
    if (combiner == MPI_COMBINER_F90_COMPLEX)      return c_f90_complex;
    if (combiner == MPI_COMBINER_F90_INTEGER)      return c_f90_integer;
    if (combiner == MPI_COMBINER_RESIZED)          return c_resized;
    
    return NULL;
}

/* --BEGIN ERROR HANDLING-- */
void MPIDU_Datatype_debug(MPI_Datatype type,
			  int array_ct)
{
    int is_builtin;
    MPID_Datatype *dtp;

    is_builtin = (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN);

    MPIU_dbg_printf("# MPIU_Datatype_debug: MPI_Datatype = 0x%0x (%s)\n", type,
		    (is_builtin) ? MPIDU_Datatype_builtin_to_string(type) :
		    "derived");

    if (is_builtin) return;

    MPID_Datatype_get_ptr(type, dtp);

    MPIU_dbg_printf("# Size = %d, Extent = %d, LB = %d%s, UB = %d%s, Extent = %d, %s\n",
		    (int) dtp->size,
		    (int) dtp->extent,
		    (int) dtp->lb,
		    (dtp->has_sticky_lb) ? "(sticky)" : "",
		    (int) dtp->ub,
		    (dtp->has_sticky_ub) ? "(sticky)" : "",
		    (int) dtp->extent,
		    dtp->is_contig ? "is N contig" : "is not N contig");

    MPIU_dbg_printf("# Contents:\n");
    MPIDI_Datatype_contents_printf(type, 0, array_ct);

    MPIU_dbg_printf("# Dataloop:\n");
    MPIDI_Datatype_dot_printf(type, 0, 1);
}

static char *MPIDI_Datatype_depth_spacing(int depth)
{
    static char d0[] = "";
    static char d1[] = "  ";
    static char d2[] = "    ";
    static char d3[] = "      ";
    static char d4[] = "        ";
    static char d5[] = "          ";

    switch (depth) {
	case 0: return d0;
	case 1: return d1;
	case 2: return d2;
	case 3: return d3;
	case 4: return d4;
	default: return d5;
    }
}

void MPIDI_Datatype_contents_printf(MPI_Datatype type,
				    int depth,
				    int acount)
{
    int i;
    MPID_Datatype *dtp;
    MPID_Datatype_contents *cp;

    MPI_Aint *aints;
    MPI_Datatype *types;
    int *ints;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	MPIU_dbg_printf("# %stype: %s\n",
			MPIDI_Datatype_depth_spacing(depth),
			MPIDU_Datatype_builtin_to_string(type));
	return;
    }

    MPID_Datatype_get_ptr(type, dtp);
    cp = dtp->contents;

    types = (MPI_Datatype *) (((char *) cp) +
			      sizeof(MPID_Datatype_contents));
    ints  = (int *) (((char *) types) +
		     cp->nr_types * sizeof(MPI_Datatype));
    aints = (MPI_Aint *) (((char *) ints) +
			  cp->nr_ints * sizeof(int));

    MPIU_dbg_printf("# %scombiner: %s\n",
		    MPIDI_Datatype_depth_spacing(depth),
		    MPIDU_Datatype_combiner_to_string(cp->combiner));

    switch (cp->combiner) {
	case MPI_COMBINER_NAMED:
	case MPI_COMBINER_DUP:
	    return;
	case MPI_COMBINER_RESIZED:
	    /* not done */
	    return;
	case MPI_COMBINER_CONTIGUOUS:
	    MPIU_dbg_printf("# %scontig ct = %d\n", 
			    MPIDI_Datatype_depth_spacing(depth),
			    *ints);
	    MPIDI_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    return;
	case MPI_COMBINER_VECTOR:
	    MPIU_dbg_printf("# %svector ct = %d, blk = %d, str = %d\n",
			    MPIDI_Datatype_depth_spacing(depth),
			    ints[0],
			    ints[1],
			    ints[2]);
	    MPIDI_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    return;
	case MPI_COMBINER_HVECTOR:
	    MPIU_dbg_printf("# %shvector ct = %d, blk = %d, str = %d\n",
			    MPIDI_Datatype_depth_spacing(depth),
			    ints[0],
			    ints[1],
			    (int) aints[0]);
	    MPIDI_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    return;
	case MPI_COMBINER_INDEXED:
	    MPIU_dbg_printf("# %sindexed ct = %d:\n",
			    MPIDI_Datatype_depth_spacing(depth),
			    ints[0]);
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPIU_dbg_printf("# %s  indexed [%d]: blk = %d, disp = %d\n",
				MPIDI_Datatype_depth_spacing(depth),
				i,
				ints[2*i+1],
				ints[2*i+2]);
		MPIDI_Datatype_contents_printf(*types,
					       depth + 1,
					       acount);
	    }
	    return;
	case MPI_COMBINER_HINDEXED:
	    MPIU_dbg_printf("# %shindexed ct = %d:\n",
			    MPIDI_Datatype_depth_spacing(depth),
			    ints[0]);
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPIU_dbg_printf("# %s  hindexed [%d]: blk = %d, disp = %d\n",
				MPIDI_Datatype_depth_spacing(depth),
				i,
				(int) ints[i+1],
				(int) aints[i]);
		MPIDI_Datatype_contents_printf(*types,
					       depth + 1,
					       acount);
	    }
	    return;
	case MPI_COMBINER_STRUCT:
	    MPIU_dbg_printf("# %sstruct ct = %d:\n",
			    MPIDI_Datatype_depth_spacing(depth),
			    (int) ints[0]);
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPIU_dbg_printf("# %s  struct[%d]: blk = %d, disp = %d\n",
				MPIDI_Datatype_depth_spacing(depth),
				i,
				(int) ints[i+1],
				(int) aints[i]);
		MPIDI_Datatype_contents_printf(types[i],
					       depth + 1,
					       acount);
	    }
	    return;
	default:
	    MPIU_dbg_printf("# %sunhandled combiner\n",
			MPIDI_Datatype_depth_spacing(depth));
	    return;
    }
}
/* --END ERROR HANDLING-- */
