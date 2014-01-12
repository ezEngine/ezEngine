/******************************************************************************

Copyright 2011 Intel Corporation All Rights Reserved.

The source code, information and material ("Material") contained herein is owned 
by Intel Corporation or its suppliers or licensors, and title to such Material 
remains with Intel Corporation or its suppliers or licensors. The Material 
contains proprietary information of Intel or its suppliers and licensors. The 
Material is protected by worldwide copyright laws and treaty provisions. No part 
of the Material may be used, copied, reproduced, modified, published, uploaded, 
posted, transmitted, distributed or disclosed in any way without Intel's prior 
express written permission. No license under any patent, copyright or other 
intellectual property rights in the Material is granted to or conferred upon you, 
either expressly, by implication, inducement, estoppel or otherwise. Any license 
under such intellectual property rights must be express and approved by Intel 
in writing.

Unless otherwise agreed by Intel in writing, you may not remove or alter this 
notice or any other notice embedded in Materials by Intel or Intel’s suppliers 
or licensors in any way

******************************************************************************/

#ifndef GPA_SDK_API
#define GPA_SDK_API
#include "ittnotify.h"

#ifndef INTEL_GPANOTIFY_PREFIX
#define INTEL_GPANOTIFY_PREFIX __gpa_
#endif

#define GPANOTIFY_NAME_AUX(n) ITT_JOIN(INTEL_GPANOTIFY_PREFIX,n)
#define GPANOTIFY_NAME(n)     ITT_VERSIONIZE(GPANOTIFY_NAME_AUX(ITT_JOIN(n,INTEL_ITTNOTIFY_POSTFIX)))

#define GPANOTIFY_VOID(n) (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)
#define GPANOTIFY_DATA(n) (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)

#define GPANOTIFY_VOID_D0(n,d)       (!(d)->flags) ? (void)0 : (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)(d)
#define GPANOTIFY_VOID_D1(n,d,x)     (!(d)->flags) ? (void)0 : (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)(d,x)
#define GPANOTIFY_VOID_D2(n,d,x,y)   (!(d)->flags) ? (void)0 : (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)(d,x,y)
#define GPANOTIFY_VOID_D3(n,d,x,y,z) (!(d)->flags) ? (void)0 : (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)(d,x,y,z)
#define GPANOTIFY_VOID_D6(n,d,x,y,z,a,b,c) (!(d)->flags) ? (void)0 : (!GPANOTIFY_NAME(n)) ? (void)0 : GPANOTIFY_NAME(n)(d,x,y,z,a,b,c)
#define GPANOTIFY_DATA_D0(n,d)       (!(d)->flags) ?       0 : (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)(d)
#define GPANOTIFY_DATA_D1(n,d,x)     (!(d)->flags) ?       0 : (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)(d,x)
#define GPANOTIFY_DATA_D2(n,d,x,y)   (!(d)->flags) ?       0 : (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)(d,x,y)
#define GPANOTIFY_DATA_D3(n,d,x,y,z) (!(d)->flags) ?       0 : (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)(d,x,y,z)
#define GPANOTIFY_DATA_D6(n,d,x,y,z,a,b,c) (!(d)->flags) ? 0 : (!GPANOTIFY_NAME(n)) ?       0 : GPANOTIFY_NAME(n)(d,x,y,z,a,b,c)

#ifdef GPA_STUB
#undef GPA_STUB
#endif
#ifdef GPA_STUBV
#undef GPA_STUBV
#endif
#define GPA_STUBV(api,type,name,args)                             \
    typedef type (api* ITT_JOIN(GPANOTIFY_NAME(name),_t)) args;   \
    extern ITT_JOIN(GPANOTIFY_NAME(name),_t) GPANOTIFY_NAME(name);
#define GPA_STUB GPA_STUBV

#ifdef __cplusplus
extern "C" {
#endif

/** @cond exclude_from_documentation */

///////////////////////////////////////////////////////////////////////////////////////////
///
///Metric description structure
///
///////////////////////////////////////////////////////////////////////////////////////////
# define __GPA_NAME_LENGTH              128
# define __GPA_DESCRIPTION_LENGTH       256
# define __GPA_URI_LENGTH               256
# define __GPA_UNITS_LENGTH             32
# define __GPA_GROUP_NAME_LENGTH        128

typedef struct ___gpa_metric
{
#if defined(UNICODE) || defined(_UNICODE)
    wchar_t Uri  [__GPA_URI_LENGTH];                 //Uri of metric
    wchar_t Name [__GPA_NAME_LENGTH];                //Name of metric
    wchar_t Description[__GPA_DESCRIPTION_LENGTH];   //Metric description
    wchar_t Units[__GPA_UNITS_LENGTH];               //Metric units
    wchar_t Group[__GPA_NAME_LENGTH];                //Metric group
#else
    char Uri  [__GPA_URI_LENGTH];                    //Uri of metric
    char Name [__GPA_NAME_LENGTH];                   //Name of metric
    char Description[__GPA_DESCRIPTION_LENGTH];      //Metric description
    char Units[__GPA_UNITS_LENGTH];                  //Metric units
#endif
    double MaxValue;                                 //Max metric value
    int    Flags;                                    //Metric visibility
    int    ProcessingFlag;                           //Metric processing flag
} __gpa_metric;

/**
 * @ingroup gpaextensions
 * @brief Register metric into the metrics processing pipeline.
 *
 * Register user metric into the metrics processing pipeline.
 *
 * @param[in] metric - The metric description; NULL pointer is possible but not expected
 */
__itt_domain* ITTAPI __gpa_register_metric(const __gpa_metric* metric);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
GPA_STUB(ITTAPI, __itt_domain*, register_metric, (const __gpa_metric* metric))
#define __gpa_register_metric     GPANOTIFY_DATA(register_metric)
#define __gpa_register_metric_ptr GPANOTIFY_NAME(register_metric)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __gpa_register_metric(metric) (__itt_domain*)0
#define __gpa_register_metric_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __gpa_register_metric_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Push metric to the metrics processing pipeline.
 *
 * Push user metric to the metrics processing pipeline.
 *
 * @param[in] domain - The domain for this marker
 * @param[in] value  - The matric value
 */
void ITTAPI __gpa_push_metric(const __itt_domain* domain, double value);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
GPA_STUBV(ITTAPI, void, push_metric, (const __itt_domain* domain, double value))
#define __gpa_push_metric(d,x)  GPANOTIFY_VOID_D1(push_metric,d,x)
#define __gpa_push_metric_ptr   GPANOTIFY_NAME(push_metric)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __gpa_push_metric(domain,value)
#define __gpa_push_metric_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __gpa_push_metric_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Set graphics marker.
 *
 * Set named and colorized marker for the Intel(R) GPA Frame Analyzer.
 *
 * @param[in] domain - The domain for this marker
 * @param[in] color  - The color for this marker.
 * @param[in] name   - The name for this marker; NULL or empty string is possible but not expected
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __gpa_gfx_set_markerA(const __itt_domain *domain, unsigned int color, const char* name);
void ITTAPI __gpa_gfx_set_markerW(const __itt_domain *domain, unsigned int color, const wchar_t* name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __gpa_gfx_set_marker     __gpa_gfx_set_markerW
#  define __gpa_gfx_set_marker_ptr __gpa_gfx_set_markerW_ptr
#else /* UNICODE */
#  define __gpa_gfx_set_marker     __gpa_gfx_set_markerA
#  define __gpa_gfx_set_marker_ptr __gpa_gfx_set_markerA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __gpa_gfx_set_marker(const __itt_domain *domain, unsigned int color, const char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
GPA_STUBV(ITTAPI, void, gfx_set_markerA, (const __itt_domain *domain, unsigned int color, const char* name))
GPA_STUBV(ITTAPI, void, gfx_set_markerW, (const __itt_domain *domain, unsigned int color, const wchar_t* name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
GPA_STUBV(ITTAPI, void, gfx_set_marker,  (const __itt_domain *domain, unsigned int color, const char* name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_set_markerA(d,x,y)  GPANOTIFY_VOID_D2(gfx_set_markerA,d,x,y)
#define __gpa_gfx_set_markerA_ptr     GPANOTIFY_NAME(gfx_set_markerA)
#define __gpa_gfx_set_markerW(d,x,y)  GPANOTIFY_VOID_D2(gfx_set_markerW,d,x,y)
#define __gpa_gfx_set_markerW_ptr     GPANOTIFY_NAME(gfx_set_markerW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_set_marker(d,x,y)   GPANOTIFY_VOID_D2(gfx_set_marker,d,x,y)
#define __gpa_gfx_set_marker_ptr      GPANOTIFY_NAME(gfx_set_marker)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_set_markerA(domain,color,name)
#define __gpa_gfx_set_markerA_ptr 0
#define __gpa_gfx_set_markerW(domain,color,name)
#define __gpa_gfx_set_markerW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_set_marker(domain,color,name)
#define __gpa_gfx_set_marker_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_set_markerA_ptr 0
#define __gpa_gfx_set_markerW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_set_marker_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Set graphics begin region marker.
 *
 * Set named and colorized begin region marker for the Intel(R) GPA Frame Analyzer.
 *
 * @param[in] domain - The domain for this marker
 * @param[in] color  - The color for this marker.
 * @param[in] name   - The name for this marker; NULL or empty string is possible but not expected
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __gpa_gfx_begin_regionA(const __itt_domain *domain, unsigned int color, const char* name);
void ITTAPI __gpa_gfx_begin_regionW(const __itt_domain *domain, unsigned int color, const wchar_t* name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __gpa_gfx_begin_region     __gpa_gfx_begin_regionW
#  define __gpa_gfx_begin_region_ptr __gpa_gfx_begin_regionW_ptr
#else /* UNICODE */
#  define __gpa_gfx_begin_region     __gpa_gfx_begin_regionA
#  define __gpa_gfx_begin_region_ptr __gpa_gfx_begin_regionA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __gpa_gfx_begin_region(const __itt_domain *domain, unsigned int color, const char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
GPA_STUBV(ITTAPI, void, gfx_begin_regionA, (const __itt_domain *domain, unsigned int color, const char* name))
GPA_STUBV(ITTAPI, void, gfx_begin_regionW, (const __itt_domain *domain, unsigned int color, const wchar_t* name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
GPA_STUBV(ITTAPI, void, gfx_begin_region,  (const __itt_domain *domain, unsigned int color, const char* name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_begin_regionA(d,x,y)  GPANOTIFY_VOID_D2(gfx_begin_regionA,d,x,y)
#define __gpa_gfx_begin_regionA_ptr     GPANOTIFY_NAME(gfx_begin_regionA)
#define __gpa_gfx_begin_regionW(d,x,y)  GPANOTIFY_VOID_D2(gfx_begin_regionW,d,x,y)
#define __gpa_gfx_begin_regionW_ptr     GPANOTIFY_NAME(gfx_begin_regionW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_begin_region(d,x,y)   GPANOTIFY_VOID_D2(gfx_begin_region,d,x,y)
#define __gpa_gfx_begin_region_ptr      GPANOTIFY_NAME(gfx_begin_region)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_begin_regionA(domain,color,name)
#define __gpa_gfx_begin_regionA_ptr 0
#define __gpa_gfx_begin_regionW(domain,color,name)
#define __gpa_gfx_begin_regionW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_begin_region(domain,color,name)
#define __gpa_gfx_begin_region_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_begin_regionA_ptr 0
#define __gpa_gfx_begin_regionW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_begin_region_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Set graphics end region marker.
 *
 * Set end region marker for the Intel(R) GPA Frame Analyzer.
 *
 * @param[in] domain - The domain for this marker
 */
void ITTAPI __gpa_gfx_end_region(const __itt_domain *domain);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
GPA_STUBV(ITTAPI, void, gfx_end_region, (const __itt_domain *domain))
#define __gpa_gfx_end_region(d)    GPANOTIFY_VOID_D0(gfx_end_region,d)
#define __gpa_gfx_end_region_ptr   GPANOTIFY_NAME(gfx_end_region)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __gpa_gfx_end_region(domain)
#define __gpa_gfx_end_region_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __gpa_gfx_end_region_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Capture graphics frame.
 *
 * Capture graphics frame for offline analysis in the Intel(R) GPA Frame Analyzer.
 *
 * @param[in] domain - The domain for this marker
 * @param[in] name   - The frame file name; must be valid; NULL or empty string are possible but not expected
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __gpa_gfx_capture_frameA(const __itt_domain *domain, const char* name);
void ITTAPI __gpa_gfx_capture_frameW(const __itt_domain *domain, const wchar_t* name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __gpa_gfx_capture_frame     __gpa_gfx_capture_frameW
#  define __gpa_gfx_capture_frame_ptr __gpa_gfx_capture_frameW_ptr
#else /* UNICODE */
#  define __gpa_gfx_capture_frame     __gpa_gfx_capture_frameA
#  define __gpa_gfx_capture_frame_ptr __gpa_gfx_capture_frameA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __gpa_gfx_capture_frame(const __itt_domain *domain, const char* name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
GPA_STUBV(ITTAPI, void, gfx_capture_frameA, (const __itt_domain *domain, const char* name))
GPA_STUBV(ITTAPI, void, gfx_capture_frameW, (const __itt_domain *domain, const wchar_t* name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
GPA_STUBV(ITTAPI, void, gfx_capture_frame,  (const __itt_domain *domain, unsigned int color, const char* name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_capture_frameA(d,x)  GPANOTIFY_VOID_D1(gfx_capture_frameA,d,x)
#define __gpa_gfx_capture_frameA_ptr   GPANOTIFY_NAME(gfx_capture_frameA)
#define __gpa_gfx_capture_frameW(d,x)  GPANOTIFY_VOID_D1(gfx_capture_frameW,d,x)
#define __gpa_gfx_capture_frameW_ptr   GPANOTIFY_NAME(gfx_capture_frameW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_capture_frame(d,x)   GPANOTIFY_VOID_D1(gfx_capture_frame,d,x)
#define __gpa_gfx_capture_frame_ptr    GPANOTIFY_NAME(gfx_capture_frame)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_capture_frameA(domain,name)
#define __gpa_gfx_capture_frameA_ptr 0
#define __gpa_gfx_capture_frameW(domain,name)
#define __gpa_gfx_capture_frameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_capture_frame(domain,name)
#define __gpa_gfx_capture_frame_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __gpa_gfx_capture_frameA_ptr 0
#define __gpa_gfx_capture_frameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __gpa_gfx_capture_frame_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Disable capturing graphics frame.
 *
 * Turn off a part of a graphics frame capturing to the Intel(R) GPA Frame Analyzer
 * frame file. This allows to capture only a required part of a rendered frame.
 *
 * @param[in] domain - The domain for this command
 */
void ITTAPI __gpa_gfx_capture_mute_on(const __itt_domain *domain);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
GPA_STUBV(ITTAPI, void, gfx_capture_mute_on, (const __itt_domain *domain))
#define __gpa_gfx_capture_mute_on(d)    GPANOTIFY_VOID_D0(gfx_capture_mute_on,d)
#define __gpa_gfx_capture_mute_on_ptr   GPANOTIFY_NAME(gfx_capture_mute_on)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __gpa_gfx_capture_mute_on(domain)
#define __gpa_gfx_capture_mute_on_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __gpa_gfx_capture_mute_on_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup gpaextensions
 * @brief Enable capturing graphics frame.
 *
 * Turn on a part of a graphics frame capturing to the Intel(R) GPA Frame Analyzer
 * frame file after it has been turned off. This allows to capture only a required
 * part of a rendered frame.
 *
 * @param[in] domain - The domain for this command
 */
void ITTAPI __gpa_gfx_capture_mute_off(const __itt_domain *domain);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
GPA_STUBV(ITTAPI, void, gfx_capture_mute_off, (const __itt_domain *domain))
#define __gpa_gfx_capture_mute_off(d)    GPANOTIFY_VOID_D0(gfx_capture_mute_off,d)
#define __gpa_gfx_capture_mute_off_ptr   GPANOTIFY_NAME(gfx_capture_mute_off)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __gpa_gfx_capture_mute_off(domain)
#define __gpa_gfx_capture_mute_off_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __gpa_gfx_capture_mute_off_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif