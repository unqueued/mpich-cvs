#ifndef __MPIDEXT32SEGMENT_H
#define __MPIDEXT32SEGMENT_H

#include "mpichconf.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_INT64
#define uint64_t __int64
#define uint32_t __int32
#endif

#if ((defined(_BIG_ENDIAN) && !defined(ntohl)) || (__BYTE_ORDER == __BIG_ENDIAN))
#define BLENDIAN 0 /* detected host arch byte order is big endian */
#else
#define BLENDIAN 1 /* detected host arch byte order is little endian */
#endif

#define FLENDIAN (__FLOAT_WORD_ORDER == __LITTLE_ENDIAN)

/*
  set to 1: uses manual swapping routines
            for 16/32 bit data types
  set to 0: uses system provided swapping routines
            for 16/32 bit data types
*/
#define MANUAL_BYTESWAPS 1

/*
  NOTE:

  There are two 'public' calls here:

  FLOAT_convert(src, dest) -- converts floating point src into
  external32 floating point format and stores the result in dest.

  BASIC_convert(src, dest) -- converts integral type src into
  external32 integral type and stores the result in dest.

  These two macros compile to assignments on big-endian architectures.
*/

#if (MANUAL_BYTESWAPS == 0)
#include <netinet/in.h>
#endif

#define BITSIZE_OF(type)    (sizeof(type) * CHAR_BIT)

#if (MANUAL_BYTESWAPS == 1)
#define BASIC_convert32(src, dest)      \
do {                                    \
    dest = (((src >> 24) & 0x000000FF) |\
            ((src >>  8) & 0x0000FF00) |\
            ((src <<  8) & 0x00FF0000) |\
            ((src << 24) & 0xFF000000));\
} while(0)
#else
#define BASIC_convert32(src, dest)      \
do {                                    \
    dest = htonl((uint32_t)src);        \
} while(0)
#endif

#if (MANUAL_BYTESWAPS == 1)
#define BASIC_convert16(src, dest)  \
do {                                \
    dest = (((src >> 8) & 0x00FF) | \
            ((src << 8) & 0xFF00)); \
} while(0)
#else
#define BASIC_convert16(src, dest)  \
do {                                \
    dest = htons((uint16_t)src);    \
} while(0)
#endif

static inline void BASIC_convert64(uint64_t *src, uint64_t *dest)
{
    uint32_t tmp_src[2];
    uint32_t tmp_dest[2];

    tmp_src[0] = (uint32_t)(*src >> 32);
    tmp_src[1] = (uint32_t)((*src << 32) >> 32);

    BASIC_convert32(tmp_src[0], tmp_dest[0]);
    BASIC_convert32(tmp_src[1], tmp_dest[1]);

    *dest = (uint64_t)tmp_dest[0];
    *dest <<= 32;
    *dest |= (uint64_t)tmp_dest[1];
}

static inline void BASIC_convert96(char *src, char *dest)
{
    uint32_t tmp_src[3];
    uint32_t tmp_dest[3];
    char *ptr = dest;

    tmp_src[0] = (uint32_t)(*((uint64_t *)src) >> 32);
    tmp_src[1] = (uint32_t)((*((uint64_t *)src) << 32) >> 32);
    tmp_src[2] = (uint32_t)
        (*((uint32_t *)((char *)src + sizeof(uint64_t))));

    BASIC_convert32(tmp_src[0], tmp_dest[0]);
    BASIC_convert32(tmp_src[1], tmp_dest[1]);
    BASIC_convert32(tmp_src[2], tmp_dest[2]);

    *((uint32_t *)ptr) = tmp_dest[0];
    ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = tmp_dest[1];
    ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = tmp_dest[2];
}

static inline void BASIC_convert128(char *src, char *dest)
{
    uint64_t tmp_src[2];
    uint64_t tmp_dest[2];
    char *ptr = dest;

    tmp_src[0] = *((uint64_t *)src);
    tmp_src[1] = *((uint64_t *)((char *)src + sizeof(uint64_t)));

    BASIC_convert64(&tmp_src[0], &tmp_dest[0]);
    BASIC_convert64(&tmp_src[1], &tmp_dest[1]);

    *((uint64_t *)ptr) = tmp_dest[0];
    ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = tmp_dest[1];
}

#if (BLENDIAN == 1)
#define BASIC_convert(src, dest)               \
do {                                           \
    register int type_byte_size = sizeof(src); \
    switch(type_byte_size)                     \
    {                                          \
        case 1:                                \
            dest = src;                        \
            break;                             \
        case 2:                                \
            BASIC_convert16(src, dest);        \
            break;                             \
        case 4:                                \
            BASIC_convert32(src, dest);        \
            break;                             \
        case 8:                                \
            BASIC_convert64((uint64_t *)&src,  \
                            (uint64_t *)&dest);\
            break;                             \
    }                                          \
} while(0)

/*
  http://www.mpi-forum.org/docs/mpi-20-html/node200.htm

  When converting a larger size integer to a smaller size integer,
  only the less significant bytes are moved. Care must be taken to
  preserve the sign bit value. This allows no conversion errors if the
  data range is within the range of the smaller size integer. ( End of
  advice to implementors.)
*/
#define BASIC_mixed_convert(src, dest)
#else
#define BASIC_convert(src, dest)               \
        do { dest = src; } while(0)
#define BASIC_mixed_convert(src, dest)         \
        do { dest = src; } while(0)
#endif

/*
  Notes on the IEEE floating point format
  ---------------------------------------

  external32 for floating point types is big-endian IEEE format.

  ---------------------
  32 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_single_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:8;
  unsigned int mantissa:23;
  };

  * little endian byte order
  struct le_ieee754_single_precision
  {
  unsigned int mantissa:23;
  unsigned int exponent:8;
  unsigned int sign_neg:1;
  };
  ---------------------

  ---------------------
  64 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_double_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:11;
  unsigned int mantissa0:20;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * big endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
  };
  ---------------------

  ---------------------
  96 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee854_double_extended
  {
  unsigned int negative:1;
  unsigned int exponent:15;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * big endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:32;
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  };
  ---------------------

  128 bit floating point implementation notes
  ===========================================

  "A 128-bit long double number consists of an ordered pair of
  64-bit double-precision numbers. The first member of the
  ordered pair contains the high-order part of the number, and
  the second member contains the low-order part. The value of the
  long double quantity is the sum of the two 64-bit numbers."

  From http://nscp.upenn.edu/aix4.3html/aixprggd/genprogc/128bit_long_double_floating-point_datatype.htm
  [ as of 09/04/2003 ]
*/

#if (BLENDIAN == 1)
#define FLOAT_convert(src, dest)              \
do {                                          \
    register int type_byte_size = sizeof(src);\
    switch(type_byte_size)                    \
    {                                         \
        case 4:                               \
        {                                     \
           long d;                            \
           BASIC_convert32((long)src, d);     \
           dest = (float)d;                   \
        }                                     \
        break;                                \
        case 8:                               \
        {                                     \
           BASIC_convert64((uint64_t *)&src,  \
                           (uint64_t *)&dest);\
        }                                     \
        case 12:                              \
        {                                     \
           BASIC_convert96((char *)&src,      \
                           (char *)&dest);    \
        }                                     \
        case 16:                              \
        {                                     \
           BASIC_convert128((char *)&src,     \
                            (char *)&dest);   \
        }                                     \
        break;                                \
    }                                         \
} while(0)
#else
#define FLOAT_convert(src, dest)              \
        do { dest = src; } while(0)
#endif

#ifdef HAVE_INT16_T
#define TWO_BYTE_BASIC_TYPE int16_t
#else
#if (SIZEOF_SHORT == 2)
#define TWO_BYTE_BASIC_TYPE short
#else
#error "Cannot detect a basic type that is 2 bytes long"
#endif
#endif /* HAVE_INT16_T */

#ifdef HAVE_INT32_T
#define FOUR_BYTE_BASIC_TYPE int32_t
#else
#if (SIZEOF_INT == 4)
#define FOUR_BYTE_BASIC_TYPE int
#elif (SIZEOF_LONG == 4)
#define FOUR_BYTE_BASIC_TYPE long
#else
#error "Cannot detect a basic type that is 4 bytes long"
#endif
#endif /* HAVE_INT32_T */

#ifdef HAVE_INT64_T
#define EIGHT_BYTE_BASIC_TYPE int64_t
#else
#ifdef HAVE_INT64
#define EIGHT_BYTE_BASIC_TYPE __int64
#elif (SIZEOF_LONG_LONG == 8)
#define EIGHT_BYTE_BASIC_TYPE long long
#else
#error "Cannot detect a basic type that is 8 bytes long"
#endif
#endif /* HAVE_INT64_T */

#if (SIZEOF_FLOAT == 4)
#define FOUR_BYTE_FLOAT_TYPE float
#else
#error "Cannot detect a float type that is 4 bytes long"
#endif

#if (SIZEOF_DOUBLE == 8)
#define EIGHT_BYTE_FLOAT_TYPE double
#else
#error "Cannot detect a float type that is 8 bytes long"
#endif

#endif /* __MPIDEXT32SEGMENT_H */
