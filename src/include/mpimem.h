#ifndef MPIMEM_H_INCLUDED
#define MPIMEM_H_INCLUDED
/* ------------------------------------------------------------------------- */
/* mpimem.h */
/* ------------------------------------------------------------------------- */
/* Memory allocation */
/* style: allow:malloc:2 sig:0 */
/* style: allow:free:2 sig:0 */
/* style: allow:strdup:2 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: define:__strdup:1 sig:0 */
/* style: define:strdup:1 sig:0 */
/* style: allow:fprintf:5 sig:0 */   /* For handle debugging ONLY */

/* Define the string copy and duplication functions */
/* Safer string routines */
int MPIU_Strncpy( char *, const char *, size_t );
int MPIU_Strnapp( char *, const char *, size_t );
char *MPIU_Strdup( const char * );

#ifdef USE_MEMORY_TRACING
#define MPIU_Malloc(a)    MPIU_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define MPIU_Calloc(a,b)  \
    MPIU_trcalloc((unsigned)(a),(unsigned)(b),__LINE__,__FILE__)
#define MPIU_Free(a)      MPIU_trfree(a,__LINE__,__FILE__)
#define MPIU_Strdup(a)    MPIU_trstrdup(a,__LINE__,__FILE__)
/* Define these as invalid C to catch their use in the code */
#define malloc(a)         'Error use MPIU_Malloc'
#define calloc(a,b)       'Error use MPIU_Calloc'
#define free(a)           'Error use MPIU_Free'
#if defined(strdup) || defined(__strdup)
#undef strdup
#endif
#define strdup(a)         'Error use MPIU_Strdup'

/* FIXME: Note that some of these prototypes are for old functions in the 
   src/util/mem/trmem.c package, and are no longer used */
void MPIU_trinit ( int );
void *MPIU_trmalloc ( unsigned int, int, const char * );
void MPIU_trfree ( void *, int, const char * );
int MPIU_trvalid ( const char * );
void MPIU_trspace ( int *, int * );
void MPIU_trid ( int );
void MPIU_trlevel ( int );
void MPIU_trpush ( int );
void MPIU_trpop (void);
void MPIU_trDebugLevel ( int );
void *MPIU_trstrdup( const char *, int, const char * );
void *MPIU_trcalloc ( unsigned, unsigned, int, const char * );
void *MPIU_trrealloc ( void *, int, int, const char * );
void MPIU_TrSetMaxMem ( int );
void MPIU_trdump ( FILE * );
void MPIU_trSummary ( FILE * );
void MPIU_trdumpGrouped ( FILE * );

#else
/* No memory tracing; just use native functions */
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#ifdef HAVE_STRDUP
#ifdef NEEDS_STRDUP_DECL
extern char *strdup( const char * );
#endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif /* HAVE_STRDUP */
#endif /* USE_MEMORY_TRACING */


/* Memory allocation stack.
   These are used to allocate multiple chunks of memory (with MPIU_Malloc)
   and ensuring that they are all freed.  This simplifies error handling
   for routines that may need to allocate multiple temporaries, and works
   on systems without alloca (allocate off of the routine's stack) */
#define MAX_MEM_STACK 16
typedef struct MPIU_Mem_stack { int n_alloc; void *ptrs[MAX_MEM_STACK]; } MPIU_Mem_stack;
#define MALLOC_STK(n,a) {a=MPIU_Malloc(n);\
               if (memstack.n_alloc >= MAX_MEM_STACK) abort(implerror);\
               memstack.ptrs[memstack.n_alloc++] = a;}
#define FREE_STK     {int i; for (i=memstack.n_alloc-1;i>=0;i--) {\
               MPIU_Free(memstack.ptrs[i]);}}
#define MALLOC_STK_INIT memstack.n_alloc = 0
#define MALLOC_STK_DECL MPIU_Mem_stack memstack

/* Utilities: Safe string copy and sprintf */
int MPIU_Strncpy( char *, const char *, size_t );
/* Provide a fallback snprintf for systems that do not have one */
#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
#else
/* Define attribute as empty if it has no definition */
#ifndef ATTRIBUTE
#define ATTRIBUTE(a)
#endif
int MPIU_Snprintf( char *str, size_t size, const char *format, ... ) 
     ATTRIBUTE((format(printf,3,4)));
#endif

/* ------------------------------------------------------------------------- */
/* end of mpimem.h */
/* ------------------------------------------------------------------------- */
#endif
