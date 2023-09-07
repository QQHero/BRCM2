/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

/******************************************************************************
 *   FileName        : bcm_BasicTypes.h 
 *   Purpose         : some basic types with accurate byte count
 *   Limitations     : None
 *   Creation Date   : 13-Sep-2001
 *   Current Release : $Name: $  
 ******************************************************************************/

#ifndef BCM_BASIC_TYPES_H
# define BCM_BASIC_TYPES_H

/* Definitions of variables of exact length. */

#ifndef NULL
#define NULL  ((void*)0)
#endif

# ifndef TRUE
#  define TRUE 1
# endif
# ifndef FALSE
#  define FALSE 0
# endif

# ifdef BCM_VCE_FAST_IN_FP
#  include "common/types.h"
typedef unsigned char Bool;
# elif __GNUC__
/* Definitions for GCC */
typedef signed char int8;
typedef signed short int int16;
typedef signed int int32;
#  ifndef BCM_UINT64_SUPPORT_REQUIRED
#   ifdef __LP64__ /* only as of GCC 3.4 */
typedef signed long int64;
#   else
typedef signed long long int int64 __attribute__ ((aligned(8)));
#   endif
#  endif
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
#  ifndef BCM_UINT64_SUPPORT_REQUIRED
#   ifdef __LP64__
typedef unsigned long uint64;
#   else
typedef unsigned long long int uint64 __attribute__ ((aligned(8)));
#   endif
#  endif
typedef unsigned char Bool;

#ifdef SUPPORT_HMI

#ifndef MAX
#define MAX(a,b) ((a)<(b) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif
//#define bool Bool
#define OFFSET(structure, member) ((long) &((structure *)0)->member)

typedef int8   fix7_1;
typedef uint8  ufix7_1;
typedef uint8  ufix0_8;
typedef int16  fix8_8;

typedef struct {
  int16 real;
  int16 imag; 
} ComplexInt16;

/* definitions for qualifying function arguments */
#define IN
#define OUT
#define INOUT

#if defined(ROC_SUPPORT)
#define NUM_MSG_LATENCY_PATHS 2
#else
#define NUM_MSG_LATENCY_PATHS 1
#endif

/* Format of bandplan used by HMI and conversion functions */
typedef struct ToneGroup ToneGroup;
struct ToneGroup
{
  uint16 endTone;
  uint16 startTone;
} BCM_PACKING_ATTRIBUTE ;

#define BCM_VDSL_NB_TONE_GROUPS 5
typedef struct BandPlanDescriptor BandPlanDescriptor;
struct BandPlanDescriptor
{
  uint8     noOfToneGroups;
  uint8     noCheck;            /* Bypass validation logic (should only be 
                                * used in lineTest and loopback modes) */
  ToneGroup toneGroups[BCM_VDSL_NB_TONE_GROUPS];
} BCM_PACKING_ATTRIBUTE ;

/* PsdTypes.h */
typedef struct BreakPointVdsl BreakPointVdsl;
struct BreakPointVdsl
{
  uint16 toneIndex;
  int16  psd; /* unit is (0.1 dB) on itf but internally 1/256 dB */
};

//#define VdslToneGroup ToneGroup

/* DefTypes.h */
typedef struct Hlini Hlini;
struct Hlini
{
  int16 a; /* a+j*b */
  int16 b; 
};

#endif  /* SUPPORT_HMI */

# elif defined(_WIN32)
/* Definitions for Visual C++ */
typedef signed __int8 int8;
typedef signed __int16 int16;
typedef signed __int32 int32;
typedef signed __int64 int64;
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
typedef unsigned __int8 Bool;
# endif

#endif
