
/* pngconf.h - machine configurable file for libpng
 *
 * libpng version 1.2.12 - June 27, 2006
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2005 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 */

/* Any machine specific code is near the front of this file, so if you
 * are configuring libpng for a machine, you may want to read the section
 * starting here down to where it starts to typedef png_color, png_text,
 * and png_info.
 */

#ifndef PNGCONF_H
#define PNGCONF_H

#define PNG_1_2_X

#include "inc/inte/FS_ExInte.h"

#ifndef PNG_VERSION_INFO_ONLY

/* End of material added to libpng-1.2.8 */

/* This is the size of the compression buffer, and thus the size of
 * an IDAT chunk.  Make this whatever size you feel is best for your
 * machine.  One of these will be allocated per png_struct.  When this
 * is full, it writes the data to the disk, and does some other
 * calculations.  Making this an extremely small size will slow
 * the library down, but you may want to experiment to determine
 * where it becomes significant, if you are concerned with memory
 * usage.  Note that zlib allocates at least 32Kb also.  For readers,
 * this describes the size of the buffer available to read the data in.
 * Unless this gets smaller than the size of a row (compressed),
 * it should not make much difference how big this is.
 */

#ifndef PNG_ZBUF_SIZE
#define PNG_ZBUF_SIZE 8192
#endif

/* Enable if you want a write-only libpng */

#ifndef PNG_NO_READ_SUPPORTED
#define PNG_READ_SUPPORTED
#endif

/* Enable if you want a read-only libpng */
#define PNG_NO_WRITE_SUPPORTED

#ifndef PNG_NO_WRITE_SUPPORTED
#define PNG_WRITE_SUPPORTED
#endif

/* Enabled by default in 1.2.0.  You can disable this if you don't need to
   support PNGs that are embedded in MNG datastreams */
#if !defined(PNG_1_0_X) && !defined(PNG_NO_MNG_FEATURES)
#  ifndef PNG_MNG_FEATURES_SUPPORTED
#    define PNG_MNG_FEATURES_SUPPORTED
#  endif
#endif

#ifndef PNG_NO_FLOATING_POINT_SUPPORTED
#  ifndef PNG_FLOATING_POINT_SUPPORTED
#    define PNG_FLOATING_POINT_SUPPORTED
#  endif
#endif

/* If you are running on a machine where you cannot allocate more
 * than 64K of memory at once, uncomment this.  While libpng will not
 * normally need that much memory in a chunk (unless you load up a very
 * large file), zlib needs to know how big of a chunk it can use, and
 * libpng thus makes sure to check any memory allocation to verify it
 * will fit into memory.
#define PNG_MAX_MALLOC_64K
 */
#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#  define PNG_MAX_MALLOC_64K
#endif

/* This macro protects us against machines that don't have function
 * prototypes (ie K&R style headers).  If your compiler does not handle
 * function prototypes, define this macro and use the included ansi2knr.
 * I've always been able to use _NO_PROTO as the indicator, but you may
 * need to drag the empty declaration out in front of here, or change the
 * ifdef to suit your own needs.
 */
#ifndef PNGARG

#ifdef OF /* zlib prototype munger */
#  define PNGARG(arglist) OF(arglist)
#else

#ifdef _NO_PROTO
#  define PNGARG(arglist) ()
#  ifndef PNG_TYPECAST_NULL
#     define PNG_TYPECAST_NULL
#  endif
#else
#  define PNGARG(arglist) arglist
#endif /* _NO_PROTO */

#endif /* OF */

#endif /* PNGARG */

#if !defined(PNG_SETJMP_NOT_SUPPORTED) && !defined(PNG_NO_SETJMP_SUPPORTED)
#  define PNG_SETJMP_SUPPORTED
#endif

#ifdef PNG_SETJMP_SUPPORTED
/* include setjmp.h for error handling */
#include <setjmp.h>
#endif /* PNG_SETJMP_SUPPORTED */

#ifdef PNG_INTERNAL
/* This controls how fine the dithering gets.  As this allocates
 * a largish chunk of memory (32K), those who are not as concerned
 * with dithering quality can decrease some or all of these.
 */
#ifndef PNG_DITHER_RED_BITS
#  define PNG_DITHER_RED_BITS 5
#endif
#ifndef PNG_DITHER_GREEN_BITS
#  define PNG_DITHER_GREEN_BITS 5
#endif
#ifndef PNG_DITHER_BLUE_BITS
#  define PNG_DITHER_BLUE_BITS 5
#endif

/* This controls how fine the gamma correction becomes when you
 * are only interested in 8 bits anyway.  Increasing this value
 * results in more memory being used, and more pow() functions
 * being called to fill in the gamma tables.  Don't set this value
 * less then 8, and even that may not work (I haven't tested it).
 */

#ifndef PNG_MAX_GAMMA_8
#  define PNG_MAX_GAMMA_8 11
#endif

/* This controls how much a difference in gamma we can tolerate before
 * we actually start doing gamma conversion.
 */
#ifndef PNG_GAMMA_THRESHOLD
#  define PNG_GAMMA_THRESHOLD 0.05
#endif

#endif /* PNG_INTERNAL */

/* Other defines for things like memory and the like can go here.  */

/* The following uses const char * instead of char * for error
 * and warning message functions, so some compilers won't complain.
 * If you do not want to use const, define PNG_NO_CONST here.
 */

#define PNG_EXTERN

#ifndef PNG_NO_CONST
#  define PNG_CONST const
#else
#  define PNG_CONST
#endif

/* The following defines give you the ability to remove code from the
 * library that you will not be using.  I wish I could figure out how to
 * automate this, but I can't do that without making it seriously hard
 * on the users.  So if you are not using an ability, change the #define
 * to and #undef, and that part of the library will not be compiled.  If
 * your linker can't find a function, you may want to make sure the
 * ability is defined here.  Some of these depend upon some others being
 * defined.  I haven't figured out all the interactions here, so you may
 * have to experiment awhile to get everything to compile.  If you are
 * creating or using a shared library, you probably shouldn't touch this,
 * as it will affect the size of the structures, and this will cause bad
 * things to happen if the library and/or application ever change.
 */

/* Any features you will not be using can be undef'ed here */

/* GR-P, 0.96a: Set "*TRANSFORMS_SUPPORTED as default but allow user
 * to turn it off with "*TRANSFORMS_NOT_SUPPORTED" or *PNG_NO_*_TRANSFORMS
 * on the compile line, then pick and choose which ones to define without
 * having to edit this file. It is safe to use the *TRANSFORMS_NOT_SUPPORTED
 * if you only want to have a png-compliant reader/writer but don't need
 * any of the extra transformations.  This saves about 80 kbytes in a
 * typical installation of the library. (PNG_NO_* form added in version
 * 1.0.1c, for consistency)
 */

/* The size of the png_text structure changed in libpng-1.0.6 when
 * iTXt support was added.  iTXt support was turned off by default through
 * libpng-1.2.x, to support old apps that malloc the png_text structure
 * instead of calling png_set_text() and letting libpng malloc it.  It
 * was turned on by default in libpng-1.3.0.
 */

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
#  ifndef PNG_NO_iTXt_SUPPORTED
#    define PNG_NO_iTXt_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_iTXt
#    define PNG_NO_READ_iTXt
#  endif
#  ifndef PNG_NO_WRITE_iTXt
#    define PNG_NO_WRITE_iTXt
#  endif
#endif

#if !defined(PNG_NO_iTXt_SUPPORTED)
#  if !defined(PNG_READ_iTXt_SUPPORTED) && !defined(PNG_NO_READ_iTXt)
#    define PNG_READ_iTXt
#  endif
#  if !defined(PNG_WRITE_iTXt_SUPPORTED) && !defined(PNG_NO_WRITE_iTXt)
#    define PNG_WRITE_iTXt
#  endif
#endif

/* The following support, added after version 1.0.0, can be turned off here en
 * masse by defining PNG_LEGACY_SUPPORTED in case you need binary compatibility
 * with old applications that require the length of png_struct and png_info
 * to remain unchanged.
 */

#ifdef PNG_LEGACY_SUPPORTED
#  define PNG_NO_FREE_ME
#  define PNG_NO_READ_UNKNOWN_CHUNKS
#  define PNG_NO_WRITE_UNKNOWN_CHUNKS
#  define PNG_NO_READ_USER_CHUNKS
#  define PNG_NO_READ_iCCP
#  define PNG_NO_WRITE_iCCP
#  define PNG_NO_READ_iTXt
#  define PNG_NO_WRITE_iTXt
#  define PNG_NO_READ_sCAL
#  define PNG_NO_WRITE_sCAL
#  define PNG_NO_READ_sPLT
#  define PNG_NO_WRITE_sPLT
#  define PNG_NO_INFO_IMAGE
#  define PNG_NO_READ_RGB_TO_GRAY
#  define PNG_NO_READ_USER_TRANSFORM
#  define PNG_NO_WRITE_USER_TRANSFORM
#  define PNG_NO_USER_MEM
#  define PNG_NO_READ_EMPTY_PLTE
#  define PNG_NO_MNG_FEATURES
#  define PNG_NO_FIXED_POINT_SUPPORTED
#endif

/* Ignore attempt to turn off both floating and fixed point support */
#if !defined(PNG_FLOATING_POINT_SUPPORTED) || \
    !defined(PNG_NO_FIXED_POINT_SUPPORTED)
#  define PNG_FIXED_POINT_SUPPORTED
#endif

#ifndef PNG_NO_FREE_ME
#  define PNG_FREE_ME_SUPPORTED
#endif

#if defined(PNG_READ_SUPPORTED)

#if !defined(PNG_READ_TRANSFORMS_NOT_SUPPORTED) && \
      !defined(PNG_NO_READ_TRANSFORMS)
#  define PNG_READ_TRANSFORMS_SUPPORTED
#endif

#define PNG_NO_READ_GAMMA
#define PNG_NO_READ_DITHER
#ifdef PNG_READ_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_READ_EXPAND
#    define PNG_READ_EXPAND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SHIFT
#    define PNG_READ_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACK
#    define PNG_READ_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_BGR
#    define PNG_READ_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP
#    define PNG_READ_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACKSWAP
#    define PNG_READ_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT
#    define PNG_READ_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_DITHER
#    define PNG_READ_DITHER_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_BACKGROUND
#    define PNG_READ_BACKGROUND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_16_TO_8
#    define PNG_READ_16_TO_8_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_FILLER
#    define PNG_READ_FILLER_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GAMMA
#    define PNG_READ_GAMMA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GRAY_TO_RGB
#    define PNG_READ_GRAY_TO_RGB_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP_ALPHA
#    define PNG_READ_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT_ALPHA
#    define PNG_READ_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_STRIP_ALPHA
#    define PNG_READ_STRIP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_USER_TRANSFORM
#    define PNG_READ_USER_TRANSFORM_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_RGB_TO_GRAY
#    define PNG_READ_RGB_TO_GRAY_SUPPORTED
#  endif
#endif /* PNG_READ_TRANSFORMS_SUPPORTED */

#if !defined(PNG_NO_PROGRESSIVE_READ) && \
 !defined(PNG_PROGRESSIVE_READ_NOT_SUPPORTED)  /* if you don't do progressive */
#  define PNG_PROGRESSIVE_READ_SUPPORTED     /* reading.  This is not talking */
#endif                               /* about interlacing capability!  You'll */
              /* still have interlacing unless you change the following line: */

#define PNG_READ_INTERLACING_SUPPORTED /* required for PNG-compliant decoders */

#ifndef PNG_NO_READ_COMPOSITE_NODIV
#  ifndef PNG_NO_READ_COMPOSITED_NODIV  /* libpng-1.0.x misspelling */
#    define PNG_READ_COMPOSITE_NODIV_SUPPORTED   /* well tested on Intel, SGI */
#  endif
#endif

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
/* Deprecated, will be removed from version 2.0.0.
   Use PNG_MNG_FEATURES_SUPPORTED instead. */
#ifndef PNG_NO_READ_EMPTY_PLTE
#  define PNG_READ_EMPTY_PLTE_SUPPORTED
#endif
#endif

#endif /* PNG_READ_SUPPORTED */

#if defined(PNG_WRITE_SUPPORTED)

# if !defined(PNG_WRITE_TRANSFORMS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_TRANSFORMS)
#  define PNG_WRITE_TRANSFORMS_SUPPORTED
#endif

#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_WRITE_SHIFT
#    define PNG_WRITE_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACK
#    define PNG_WRITE_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_BGR
#    define PNG_WRITE_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_SWAP
#    define PNG_WRITE_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACKSWAP
#    define PNG_WRITE_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT
#    define PNG_WRITE_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_FILLER
#    define PNG_WRITE_FILLER_SUPPORTED   /* same as WRITE_STRIP_ALPHA */
#  endif
#  ifndef PNG_NO_WRITE_SWAP_ALPHA
#    define PNG_WRITE_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT_ALPHA
#    define PNG_WRITE_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_USER_TRANSFORM
#    define PNG_WRITE_USER_TRANSFORM_SUPPORTED
#  endif
#endif /* PNG_WRITE_TRANSFORMS_SUPPORTED */

#if !defined(PNG_NO_WRITE_INTERLACING_SUPPORTED) && \
    !defined(PNG_WRITE_INTERLACING_SUPPORTED)
#define PNG_WRITE_INTERLACING_SUPPORTED  /* not required for PNG-compliant
                                            encoders, but can cause trouble
                                            if left undefined */
#endif

#if !defined(PNG_NO_WRITE_WEIGHTED_FILTER) && \
    !defined(PNG_WRITE_WEIGHTED_FILTER) && \
     defined(PNG_FLOATING_POINT_SUPPORTED)
#  define PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
#endif

#ifndef PNG_NO_WRITE_FLUSH
#  define PNG_WRITE_FLUSH_SUPPORTED
#endif

#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
/* Deprecated, see PNG_MNG_FEATURES_SUPPORTED, above */
#ifndef PNG_NO_WRITE_EMPTY_PLTE
#  define PNG_WRITE_EMPTY_PLTE_SUPPORTED
#endif
#endif

#endif /* PNG_WRITE_SUPPORTED */

#ifndef PNG_1_0_X
#  ifndef PNG_NO_ERROR_NUMBERS
#    define PNG_ERROR_NUMBERS_SUPPORTED
#  endif
#endif /* PNG_1_0_X */

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
#  ifndef PNG_NO_USER_TRANSFORM_PTR
#    define PNG_USER_TRANSFORM_PTR_SUPPORTED
#  endif
#endif

#define PNG_NO_CONSOLE_IO
#define PNG_NO_STDIO

#ifndef PNG_NO_STDIO
#  define PNG_TIME_RFC1123_SUPPORTED
#endif

/* This adds extra functions in pngget.c for accessing data from the
 * info pointer (added in version 0.99)
 * png_get_image_width()
 * png_get_image_height()
 * png_get_bit_depth()
 * png_get_color_type()
 * png_get_compression_type()
 * png_get_filter_type()
 * png_get_interlace_type()
 * png_get_pixel_aspect_ratio()
 * png_get_pixels_per_meter()
 * png_get_x_offset_pixels()
 * png_get_y_offset_pixels()
 * png_get_x_offset_microns()
 * png_get_y_offset_microns()
 */
#if !defined(PNG_NO_EASY_ACCESS) && !defined(PNG_EASY_ACCESS_SUPPORTED)
#  define PNG_EASY_ACCESS_SUPPORTED
#endif

/* If you are sure that you don't need thread safety and you are compiling
   with PNG_USE_PNGCCRD for an MMX application, you can define this for
   faster execution.  See pnggccrd.c.
*/
#define PNG_THREAD_UNSAFE_OK

#if !defined(PNG_1_0_X)
#if !defined(PNG_NO_USER_MEM) && !defined(PNG_USER_MEM_SUPPORTED)
#  define PNG_USER_MEM_SUPPORTED
#endif
#endif /* PNG_1_0_X */

/* Added at libpng-1.2.6 */
#if !defined(PNG_1_0_X)
#ifndef PNG_SET_USER_LIMITS_SUPPORTED
#if !defined(PNG_NO_SET_USER_LIMITS) && !defined(PNG_SET_USER_LIMITS_SUPPORTED)
#  define PNG_SET_USER_LIMITS_SUPPORTED
#endif
#endif
#endif /* PNG_1_0_X */

/* Added at libpng-1.0.16 and 1.2.6.  To accept all valid PNGS no matter
 * how large, set these limits to 0x7fffffffL
 */
#ifndef PNG_USER_WIDTH_MAX
#  define PNG_USER_WIDTH_MAX 1000000L
#endif
#ifndef PNG_USER_HEIGHT_MAX
#  define PNG_USER_HEIGHT_MAX 1000000L
#endif

/* These are currently experimental features, define them if you want */

/* very little testing */
/*
#ifdef PNG_READ_SUPPORTED
#  ifndef PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#    define PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#  endif
#endif
*/

/* This is only for PowerPC big-endian and 680x0 systems */
/* some testing */
/*
#ifndef PNG_READ_BIG_ENDIAN_SUPPORTED
#  define PNG_READ_BIG_ENDIAN_SUPPORTED
#endif
*/

/* Buggy compilers (e.g., gcc 2.7.2.2) need this */
/*
#define PNG_NO_POINTER_INDEXING
*/

/* These functions are turned off by default, as they will be phased out. */
/*
#define  PNG_USELESS_TESTS_SUPPORTED
#define  PNG_CORRECT_PALETTE_SUPPORTED
*/

/* Any chunks you are not interested in, you can undef here.  The
 * ones that allocate memory may be expecially important (hIST,
 * tEXt, zTXt, tRNS, pCAL).  Others will just save time and make png_info
 * a bit smaller.
 */

#if defined(PNG_READ_SUPPORTED) && \
    !defined(PNG_READ_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_READ_ANCILLARY_CHUNKS)
#  define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#endif

#if defined(PNG_WRITE_SUPPORTED) && \
    !defined(PNG_WRITE_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_ANCILLARY_CHUNKS)
#  define PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED
#endif

#ifdef PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#define PNG_NO_READ_sCAL

#ifdef PNG_NO_READ_TEXT
#  define PNG_NO_READ_iTXt
#  define PNG_NO_READ_tEXt
#  define PNG_NO_READ_zTXt
#endif
#ifndef PNG_NO_READ_bKGD
#  define PNG_READ_bKGD_SUPPORTED
#  define PNG_bKGD_SUPPORTED
#endif
#ifndef PNG_NO_READ_cHRM
#  define PNG_READ_cHRM_SUPPORTED
#  define PNG_cHRM_SUPPORTED
#endif
#ifndef PNG_NO_READ_gAMA
#  define PNG_READ_gAMA_SUPPORTED
#  define PNG_gAMA_SUPPORTED
#endif
#ifndef PNG_NO_READ_hIST
#  define PNG_READ_hIST_SUPPORTED
#  define PNG_hIST_SUPPORTED
#endif
#ifndef PNG_NO_READ_iCCP
#  define PNG_READ_iCCP_SUPPORTED
#  define PNG_iCCP_SUPPORTED
#endif
#ifndef PNG_NO_READ_iTXt
#  ifndef PNG_READ_iTXt_SUPPORTED
#    define PNG_READ_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_READ_oFFs
#  define PNG_READ_oFFs_SUPPORTED
#  define PNG_oFFs_SUPPORTED
#endif
#ifndef PNG_NO_READ_pCAL
#  define PNG_READ_pCAL_SUPPORTED
#  define PNG_pCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_sCAL
#  define PNG_READ_sCAL_SUPPORTED
#  define PNG_sCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_pHYs
#  define PNG_READ_pHYs_SUPPORTED
#  define PNG_pHYs_SUPPORTED
#endif
#ifndef PNG_NO_READ_sBIT
#  define PNG_READ_sBIT_SUPPORTED
#  define PNG_sBIT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sPLT
#  define PNG_READ_sPLT_SUPPORTED
#  define PNG_sPLT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sRGB
#  define PNG_READ_sRGB_SUPPORTED
#  define PNG_sRGB_SUPPORTED
#endif
#ifndef PNG_NO_READ_tEXt
#  define PNG_READ_tEXt_SUPPORTED
#  define PNG_tEXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_tIME
#  define PNG_READ_tIME_SUPPORTED
#  define PNG_tIME_SUPPORTED
#endif
#ifndef PNG_NO_READ_tRNS
#  define PNG_READ_tRNS_SUPPORTED
#  define PNG_tRNS_SUPPORTED
#endif
#ifndef PNG_NO_READ_zTXt
#  define PNG_READ_zTXt_SUPPORTED
#  define PNG_zTXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_UNKNOWN_CHUNKS
#  define PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_NO_HANDLE_AS_UNKNOWN
#    define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#  endif
#endif
#if !defined(PNG_NO_READ_USER_CHUNKS) && \
     defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
#  define PNG_READ_USER_CHUNKS_SUPPORTED
#  define PNG_USER_CHUNKS_SUPPORTED
#  ifdef PNG_NO_READ_UNKNOWN_CHUNKS
#    undef PNG_NO_READ_UNKNOWN_CHUNKS
#  endif
#  ifdef PNG_NO_HANDLE_AS_UNKNOWN
#    undef PNG_NO_HANDLE_AS_UNKNOWN
#  endif
#endif
#ifndef PNG_NO_READ_OPT_PLTE
#  define PNG_READ_OPT_PLTE_SUPPORTED /* only affects support of the */
#endif                      /* optional PLTE chunk in RGB and RGBA images */
#if defined(PNG_READ_iTXt_SUPPORTED) || defined(PNG_READ_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED)
#  define PNG_READ_TEXT_SUPPORTED
#  define PNG_TEXT_SUPPORTED
#endif

#endif /* PNG_READ_ANCILLARY_CHUNKS_SUPPORTED */

#ifdef PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED

#ifdef PNG_NO_WRITE_TEXT
#  define PNG_NO_WRITE_iTXt
#  define PNG_NO_WRITE_tEXt
#  define PNG_NO_WRITE_zTXt
#endif
#ifndef PNG_NO_WRITE_bKGD
#  define PNG_WRITE_bKGD_SUPPORTED
#  ifndef PNG_bKGD_SUPPORTED
#    define PNG_bKGD_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_cHRM
#  define PNG_WRITE_cHRM_SUPPORTED
#  ifndef PNG_cHRM_SUPPORTED
#    define PNG_cHRM_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_gAMA
#  define PNG_WRITE_gAMA_SUPPORTED
#  ifndef PNG_gAMA_SUPPORTED
#    define PNG_gAMA_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_hIST
#  define PNG_WRITE_hIST_SUPPORTED
#  ifndef PNG_hIST_SUPPORTED
#    define PNG_hIST_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iCCP
#  define PNG_WRITE_iCCP_SUPPORTED
#  ifndef PNG_iCCP_SUPPORTED
#    define PNG_iCCP_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iTXt
#  ifndef PNG_WRITE_iTXt_SUPPORTED
#    define PNG_WRITE_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_oFFs
#  define PNG_WRITE_oFFs_SUPPORTED
#  ifndef PNG_oFFs_SUPPORTED
#    define PNG_oFFs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pCAL
#  define PNG_WRITE_pCAL_SUPPORTED
#  ifndef PNG_pCAL_SUPPORTED
#    define PNG_pCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sCAL
#  define PNG_WRITE_sCAL_SUPPORTED
#  ifndef PNG_sCAL_SUPPORTED
#    define PNG_sCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pHYs
#  define PNG_WRITE_pHYs_SUPPORTED
#  ifndef PNG_pHYs_SUPPORTED
#    define PNG_pHYs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sBIT
#  define PNG_WRITE_sBIT_SUPPORTED
#  ifndef PNG_sBIT_SUPPORTED
#    define PNG_sBIT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sPLT
#  define PNG_WRITE_sPLT_SUPPORTED
#  ifndef PNG_sPLT_SUPPORTED
#    define PNG_sPLT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sRGB
#  define PNG_WRITE_sRGB_SUPPORTED
#  ifndef PNG_sRGB_SUPPORTED
#    define PNG_sRGB_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tEXt
#  define PNG_WRITE_tEXt_SUPPORTED
#  ifndef PNG_tEXt_SUPPORTED
#    define PNG_tEXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tIME
#  define PNG_WRITE_tIME_SUPPORTED
#  ifndef PNG_tIME_SUPPORTED
#    define PNG_tIME_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tRNS
#  define PNG_WRITE_tRNS_SUPPORTED
#  ifndef PNG_tRNS_SUPPORTED
#    define PNG_tRNS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_zTXt
#  define PNG_WRITE_zTXt_SUPPORTED
#  ifndef PNG_zTXt_SUPPORTED
#    define PNG_zTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_UNKNOWN_CHUNKS
#  define PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_NO_HANDLE_AS_UNKNOWN
#     ifndef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#       define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#     endif
#  endif
#endif
#if defined(PNG_WRITE_iTXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_WRITE_zTXt_SUPPORTED)
#  define PNG_WRITE_TEXT_SUPPORTED
#  ifndef PNG_TEXT_SUPPORTED
#    define PNG_TEXT_SUPPORTED
#  endif
#endif

#endif /* PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED */

/* Turn this off to disable png_read_png() and
 * png_write_png() and leave the row_pointers member
 * out of the info structure.
 */
#ifndef PNG_NO_INFO_IMAGE
#  define PNG_INFO_IMAGE_SUPPORTED
#endif

/* Some typedefs to get us started.  These should be safe on most of the
 * common platforms.  The typedefs should be at least as large as the
 * numbers suggest (a png_uint_32 must be at least 32 bits long), but they
 * don't have to be exactly that size.  Some compilers dislike passing
 * unsigned shorts as function parameters, so you may be better off using
 * unsigned int for png_uint_16.  Likewise, for 64-bit systems, you may
 * want to have unsigned int for png_uint_32 instead of unsigned long.
 */

typedef unsigned long png_uint_32;
typedef long png_int_32;
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

/* This is usually size_t.  It is typedef'ed just in case you need it to
   change (I'm not sure if you will or not, so I thought I'd be safe) */
#ifdef PNG_SIZE_T
   typedef PNG_SIZE_T png_size_t;
#  define png_sizeof(x) png_convert_size(sizeof (x))
#else
   typedef size_t png_size_t;
#  define png_sizeof(x) sizeof (x)
#endif

/* SJT: default case */
#ifndef FAR
#  define FAR
#endif

/* At this point FAR is always defined */
#ifndef FARDATA
#  define FARDATA
#endif

/* Typedef for floating-point numbers that are converted
   to fixed-point with a multiple of 100,000, e.g., int_gamma */
typedef png_int_32 png_fixed_point;

/* Add typedefs for pointers */
typedef void            FAR * png_voidp;
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;
typedef png_fixed_point FAR * png_fixed_point_p;

#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * png_doublep;
#endif

/* Pointers to pointers; i.e. arrays */
typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef png_fixed_point FAR * FAR * png_fixed_point_pp;
#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * FAR * png_doublepp;
#endif

/* Pointers to pointers to pointers; i.e., pointer to array */
typedef char            FAR * FAR * FAR * png_charppp;

#if defined(PNG_1_0_X) || defined(PNG_1_2_X)
/* SPC -  Is this stuff deprecated? */
/* It'll be removed as of libpng-1.3.0 - GR-P */
/* libpng typedefs for types in zlib. If zlib changes
 * or another compression library is used, then change these.
 * Eliminates need to change all the source files.
 */
typedef charf *         png_zcharp;
typedef charf * FAR *   png_zcharpp;
typedef z_stream FAR *  png_zstreamp;
#endif /* (PNG_1_0_X) || defined(PNG_1_2_X) */

/* Do not use global arrays (helps with building DLL's)
 * They are no longer used in libpng itself, since version 1.0.5c,
 * but might be required for some pre-1.0.5c applications.
 */
#if !defined(PNG_USE_LOCAL_ARRAYS) && !defined(PNG_USE_GLOBAL_ARRAYS)
#  if defined(PNG_NO_GLOBAL_ARRAYS) || (defined(__GNUC__) && defined(PNG_DLL))
#    define PNG_USE_LOCAL_ARRAYS
#  else
#    define PNG_USE_GLOBAL_ARRAYS
#  endif
#endif

#ifndef PNGAPI
#  define PNGAPI
#endif

#ifndef PNG_IMPEXP
#  define PNG_IMPEXP
#endif

#ifndef PNG_EXPORT
#  define PNG_EXPORT(type,symbol) PNG_IMPEXP type PNGAPI symbol
#endif

#ifdef PNG_USE_GLOBAL_ARRAYS
#  ifndef PNG_EXPORT_VAR
#    define PNG_EXPORT_VAR(type) extern PNG_IMPEXP type
#  endif
#endif

/* User may want to use these so they are not in PNG_INTERNAL. Any library
 * functions that are passed far data must be model independent.
 */

#ifndef PNG_ABORT
#  define PNG_ABORT() 
#endif

#ifdef PNG_SETJMP_SUPPORTED
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#else
#  define png_jmpbuf(png_ptr) \
   (LIBPNG_WAS_COMPILED_WITH__PNG_SETJMP_NOT_SUPPORTED)
#endif

#  define CVT_PTR(ptr)         		(ptr)
#  define CVT_PTR_NOCHECK(ptr) 		(ptr)
#  define png_strcpy  strcpy
#  define png_strncpy strncpy     /* Added to v 1.2.6 */
#  define png_strlen  strlen
#  define png_memcmp  memcmp      /* SJT: added */
#  define png_memcpy  memcpy
#  define png_memset  memset

/* Just a little check that someone hasn't tried to define something
 * contradictory.
 */
#if (PNG_ZBUF_SIZE > 65536L) && defined(PNG_MAX_MALLOC_64K)
#  undef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 65536L
#endif

#ifdef PNG_READ_SUPPORTED
/* Prior to libpng-1.0.9, this block was in pngasmrd.h */
#if defined(PNG_INTERNAL)

/* These are the default thresholds before the MMX code kicks in; if either
 * rowbytes or bitdepth is below the threshold, plain C code is used.  These
 * can be overridden at runtime via the png_set_mmx_thresholds() call in
 * libpng 1.2.0 and later.  The values below were chosen by Intel.
 */

#ifndef PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT
#  define PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT  128  /*  >=  */
#endif
#ifndef PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT
#  define PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT  9    /*  >=  */   
#endif

/* Set this in the makefile for VC++ on Pentium, not here. */
/* Platform must be Pentium.  Makefile must assemble and load pngvcrd.c .
 * MMX will be detected at run time and used if present.
 */
#ifdef PNG_USE_PNGVCRD
#  define PNG_HAVE_ASSEMBLER_COMBINE_ROW
#  define PNG_HAVE_ASSEMBLER_READ_INTERLACE
#  define PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
#endif

/* Set this in the makefile for gcc/as on Pentium, not here. */
/* Platform must be Pentium.  Makefile must assemble and load pnggccrd.c .
 * MMX will be detected at run time and used if present.
 */
#ifdef PNG_USE_PNGGCCRD
#  define PNG_HAVE_ASSEMBLER_COMBINE_ROW
#  define PNG_HAVE_ASSEMBLER_READ_INTERLACE
#  define PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
#endif
/* - see pnggccrd.c for info about what is currently enabled */

#endif /* PNG_INTERNAL */
#endif /* PNG_READ_SUPPORTED */

/* Added at libpng-1.2.8 */
#endif /* PNG_VERSION_INFO_ONLY */

#endif /* PNGCONF_H */
