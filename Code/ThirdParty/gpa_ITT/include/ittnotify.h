/*
    Copyright(c) 2005-2011 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/
#ifndef _ITTNOTIFY_H_
#define _ITTNOTIFY_H_

/** @cond exclude_from_documentation */
#ifndef ITT_OS_WIN
#  define ITT_OS_WIN   1
#endif /* ITT_OS_WIN */

#ifndef ITT_OS_LINUX
#  define ITT_OS_LINUX 2
#endif /* ITT_OS_LINUX */

#ifndef ITT_OS_MAC
#  define ITT_OS_MAC   3
#endif /* ITT_OS_MAC */

#ifndef ITT_OS
#  if defined WIN32 || defined _WIN32
#    define ITT_OS ITT_OS_WIN
#  elif defined( __APPLE__ ) && defined( __MACH__ )
#    define ITT_OS ITT_OS_MAC
#  else
#    define ITT_OS ITT_OS_LINUX
#  endif
#endif /* ITT_OS */

#ifndef ITT_PLATFORM_WIN
#  define ITT_PLATFORM_WIN 1
#endif /* ITT_PLATFORM_WIN */

#ifndef ITT_PLATFORM_POSIX
#  define ITT_PLATFORM_POSIX 2
#endif /* ITT_PLATFORM_POSIX */

#ifndef ITT_PLATFORM
#  if ITT_OS==ITT_OS_WIN
#    define ITT_PLATFORM ITT_PLATFORM_WIN
#  else
#    define ITT_PLATFORM ITT_PLATFORM_POSIX
#  endif /* _WIN32 */
#endif /* ITT_PLATFORM */

#if defined(_UNICODE) && !defined(UNICODE)
#define UNICODE
#endif

#include <stddef.h>
#include <stdarg.h>
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#include <tchar.h>
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if defined(UNICODE) || defined(_UNICODE)
#include <wchar.h>
#endif
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#ifdef CDECL
#undef CDECL
#endif /* CDECL */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#  define CDECL __cdecl
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#  define CDECL /* nothing */
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#ifdef STDCALL
#undef STDCALL
#endif /* STDCALL */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#  define STDCALL __stdcall
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#  define STDCALL /* nothing */
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#define ITTAPI    CDECL
#define LIBITTAPI CDECL

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define INLINE __forceinline // use __forceinline (VC++ specific)
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define INLINE static inline // use standard inline
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

#ifdef INTEL_ITTNOTIFY_ENABLE_LEGACY
#  if ITT_PLATFORM==ITT_PLATFORM_WIN
#    pragma message("WARNING!!! Deprecated API is used. Please undefine INTEL_ITTNOTIFY_ENABLE_LEGACY macro")
#  else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#    warning "Deprecated API is used. Please undefine INTEL_ITTNOTIFY_ENABLE_LEGACY macro"
#  endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#  include "legacy/ittnotify.h"
#endif /* INTEL_ITTNOTIFY_ENABLE_LEGACY */

// Helper macro for joining tokens
#define ITT_JOIN_AUX(p,n) p##n
#define ITT_JOIN(p,n)     ITT_JOIN_AUX(p,n)

#ifdef ITT_MAJOR
#undef ITT_MAJOR
#endif
#ifdef ITT_MINOR
#undef ITT_MINOR
#endif
#define ITT_MAJOR     3
#define ITT_MINOR     0

// Standard versioning of a token with major and minor version numbers
#define ITT_VERSIONIZE(x)    \
    ITT_JOIN(x,              \
    ITT_JOIN(_,              \
    ITT_JOIN(ITT_MAJOR,      \
    ITT_JOIN(_, ITT_MINOR))))

#ifndef INTEL_ITTNOTIFY_PREFIX
#  define INTEL_ITTNOTIFY_PREFIX __itt_
#endif /* INTEL_ITTNOTIFY_PREFIX */
#ifndef INTEL_ITTNOTIFY_POSTFIX
#  define INTEL_ITTNOTIFY_POSTFIX _ptr_
#endif /* INTEL_ITTNOTIFY_POSTFIX */

#define ITTNOTIFY_NAME_AUX(n) ITT_JOIN(INTEL_ITTNOTIFY_PREFIX,n)
#define ITTNOTIFY_NAME(n)     ITT_VERSIONIZE(ITTNOTIFY_NAME_AUX(ITT_JOIN(n,INTEL_ITTNOTIFY_POSTFIX)))

#define ITTNOTIFY_VOID(n) (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)
#define ITTNOTIFY_DATA(n) (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)

#define ITTNOTIFY_VOID_D0(n,d)       (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d)
#define ITTNOTIFY_VOID_D1(n,d,x)     (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x)
#define ITTNOTIFY_VOID_D2(n,d,x,y)   (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x,y)
#define ITTNOTIFY_VOID_D3(n,d,x,y,z) (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x,y,z)
#define ITTNOTIFY_VOID_D4(n,d,x,y,z,a)     (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x,y,z,a)
#define ITTNOTIFY_VOID_D5(n,d,x,y,z,a,b)   (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x,y,z,a,b)
#define ITTNOTIFY_VOID_D6(n,d,x,y,z,a,b,c) (!(d)->flags) ? (void)0 : (!ITTNOTIFY_NAME(n)) ? (void)0 : ITTNOTIFY_NAME(n)(d,x,y,z,a,b,c)
#define ITTNOTIFY_DATA_D0(n,d)       (!(d)->flags) ?       0 : (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)(d)
#define ITTNOTIFY_DATA_D1(n,d,x)     (!(d)->flags) ?       0 : (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)(d,x)
#define ITTNOTIFY_DATA_D2(n,d,x,y)   (!(d)->flags) ?       0 : (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)(d,x,y)
#define ITTNOTIFY_DATA_D3(n,d,x,y,z) (!(d)->flags) ?       0 : (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)(d,x,y,z)
#define ITTNOTIFY_DATA_D6(n,d,x,y,z,a,b,c) (!(d)->flags) ? 0 : (!ITTNOTIFY_NAME(n)) ?       0 : ITTNOTIFY_NAME(n)(d,x,y,z,a,b,c)

#ifdef ITT_STUB
#undef ITT_STUB
#endif
#ifdef ITT_STUBV
#undef ITT_STUBV
#endif
#define ITT_STUBV(api,type,name,args)                             \
    typedef type (api* ITT_JOIN(ITTNOTIFY_NAME(name),_t)) args;   \
    extern ITT_JOIN(ITTNOTIFY_NAME(name),_t) ITTNOTIFY_NAME(name);
#define ITT_STUB ITT_STUBV

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/** @endcond */

/** @cond exclude_from_gpa_documentation */
/**
 * @defgroup control Collection Control
 * @ingroup public
 * General behavior: application continues to run, but no profiling information is being collected
 *
 * Pausing occurs not only for the current thread but for all process as well as spawned processes
 * - Intel(R) Parallel Inspector:
 *   - Does not analyze or report errors that involve memory access.
 *   - Other errors are reported as usual. Pausing data collection in
 *     Intel(R) Parallel Inspector only pauses tracing and analyzing
 *     memory access. It does not pause tracing or analyzing threading APIs.
 *   .
 * - Intel(R) Parallel Amplifier:
 *   - Does continue to record when new threads are started.
 *   .
 * - Other effects:
 *   - Possible reduction of runtime overhead.
 *   .
 * @{
 */
/** @brief Pause collection. */
void ITTAPI __itt_pause(void);
/** @brief Resume collection. */
void ITTAPI __itt_resume(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, pause,  (void))
ITT_STUBV(ITTAPI, void, resume, (void))
#define __itt_pause      ITTNOTIFY_VOID(pause)
#define __itt_pause_ptr  ITTNOTIFY_NAME(pause)
#define __itt_resume     ITTNOTIFY_VOID(resume)
#define __itt_resume_ptr ITTNOTIFY_NAME(resume)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_pause()
#define __itt_pause_ptr  0
#define __itt_resume()
#define __itt_resume_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_pause_ptr  0
#define __itt_resume_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} control group */
/** @endcond */

/**
 * @ingroup threads
 * @brief Sets thread name using char or Unicode string.
 * @param[in] name - name of thread
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __itt_thread_set_nameA(const char    *name);
void ITTAPI __itt_thread_set_nameW(const wchar_t *name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_thread_set_name     __itt_thread_set_nameW
#  define __itt_thread_set_name_ptr       __itt_thread_set_nameW_ptr
#else /* UNICODE */
#  define __itt_thread_set_name     __itt_thread_set_nameA
#  define __itt_thread_set_name_ptr       __itt_thread_set_nameA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __itt_thread_set_name(const char *name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(ITTAPI, void, thread_set_nameA, (const char    *name))
ITT_STUBV(ITTAPI, void, thread_set_nameW, (const wchar_t *name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUBV(ITTAPI, void, thread_set_name,  (const char    *name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_thread_set_nameA     ITTNOTIFY_VOID(thread_set_nameA)
#define __itt_thread_set_nameA_ptr ITTNOTIFY_NAME(thread_set_nameA)
#define __itt_thread_set_nameW     ITTNOTIFY_VOID(thread_set_nameW)
#define __itt_thread_set_nameW_ptr ITTNOTIFY_NAME(thread_set_nameW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_thread_set_name     ITTNOTIFY_VOID(thread_set_name)
#define __itt_thread_set_name_ptr ITTNOTIFY_NAME(thread_set_name)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_thread_set_nameA(name)
#define __itt_thread_set_nameA_ptr 0
#define __itt_thread_set_nameW(name)
#define __itt_thread_set_nameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_thread_set_name(name)
#define __itt_thread_set_name_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_thread_set_nameA_ptr 0
#define __itt_thread_set_nameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_thread_set_name_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_gpa_documentation */
/**
 * @brief Mark current thread as ignored from this point on, for the duration of its existence.
 */
void ITTAPI __itt_thread_ignore(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, thread_ignore, (void))
#define __itt_thread_ignore     ITTNOTIFY_VOID(thread_ignore)
#define __itt_thread_ignore_ptr ITTNOTIFY_NAME(thread_ignore)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_thread_ignore()
#define __itt_thread_ignore_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_thread_ignore_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @defgroup sync Synchronization
 * @ingroup public
 * Synchronization group
 * @{
 */
/**
 * @hideinitializer
 * @brief Possible value of attribute argument for sync object type.
 */
#define __itt_attr_barrier 1

/**
 * @hideinitializer
 * @brief Possible value of attribute argument for sync object type.
 */
#define __itt_attr_mutex   2

/**
 * @brief Register the creation of a sync object using char or Unicode string.
 * @param[in] addr      - pointer to the sync object. You should use a real pointer to your object
 *                        to make sure that the values don't clash with other object addresses
 * @param[in] objtype   - null-terminated object type string. If NULL is passed, the object will
 *                        be assumed to be of generic "User Synchronization" type
 * @param[in] objname   - null-terminated object name string. If NULL, no name will be assigned
 *                        to the object -- you can use the __itt_sync_rename call later to assign
 *                        the name
 * @param[in] attribute - one of [#__itt_attr_barrier, #__itt_attr_mutex] values which defines the
 *                        exact semantics of how prepare/acquired/releasing calls work.
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __itt_sync_createA(void *addr, const char    *objtype, const char    *objname, int attribute);
void ITTAPI __itt_sync_createW(void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_sync_create     __itt_sync_createW
#  define __itt_sync_create_ptr __itt_sync_createW_ptr
#else /* UNICODE */
#  define __itt_sync_create     __itt_sync_createA
#  define __itt_sync_create_ptr __itt_sync_createA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __itt_sync_create (void *addr, const char *objtype, const char *objname, int attribute);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(ITTAPI, void, sync_createA, (void *addr, const char    *objtype, const char    *objname, int attribute))
ITT_STUBV(ITTAPI, void, sync_createW, (void *addr, const wchar_t *objtype, const wchar_t *objname, int attribute))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUBV(ITTAPI, void, sync_create,  (void *addr, const char*    objtype, const char*    objname, int attribute))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_createA     ITTNOTIFY_VOID(sync_createA)
#define __itt_sync_createA_ptr ITTNOTIFY_NAME(sync_createA)
#define __itt_sync_createW     ITTNOTIFY_VOID(sync_createW)
#define __itt_sync_createW_ptr ITTNOTIFY_NAME(sync_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_create     ITTNOTIFY_VOID(sync_create)
#define __itt_sync_create_ptr ITTNOTIFY_NAME(sync_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_createA(addr, objtype, objname, attribute)
#define __itt_sync_createA_ptr 0
#define __itt_sync_createW(addr, objtype, objname, attribute)
#define __itt_sync_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_create(addr, objtype, objname, attribute)
#define __itt_sync_create_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_createA_ptr 0
#define __itt_sync_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_create_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Assign a name to a sync object using char or Unicode string.
 *
 * Sometimes you cannot assign the name to a sync object in the __itt_sync_set_name() call because it
 * is not yet known there. In this case you should use the rename call which allows to assign the
 * name after the creation has been registered. The renaming can be done multiple times. All waits
 * after a new name has been assigned will be attributed to the sync object with this name.
 * @param[in] addr - pointer to the sync object
 * @param[in] name - null-terminated object name string
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __itt_sync_renameA(void *addr, const char    *name);
void ITTAPI __itt_sync_renameW(void *addr, const wchar_t *name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_sync_rename     __itt_sync_renameW
#  define __itt_sync_rename_ptr __itt_sync_renameW_ptr
#else /* UNICODE */
#  define __itt_sync_rename     __itt_sync_renameA
#  define __itt_sync_rename_ptr __itt_sync_renameA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __itt_sync_rename(void *addr, const char *name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(ITTAPI, void, sync_renameA, (void *addr, const char    *name))
ITT_STUBV(ITTAPI, void, sync_renameW, (void *addr, const wchar_t *name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUBV(ITTAPI, void, sync_rename,  (void *addr, const char    *name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_renameA     ITTNOTIFY_VOID(sync_renameA)
#define __itt_sync_renameA_ptr ITTNOTIFY_NAME(sync_renameA)
#define __itt_sync_renameW     ITTNOTIFY_VOID(sync_renameW)
#define __itt_sync_renameW_ptr ITTNOTIFY_NAME(sync_renameW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_rename     ITTNOTIFY_VOID(sync_rename)
#define __itt_sync_rename_ptr ITTNOTIFY_NAME(sync_rename)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_renameA(addr, name)
#define __itt_sync_renameA_ptr 0
#define __itt_sync_renameW(addr, name)
#define __itt_sync_renameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_rename(addr, name)
#define __itt_sync_rename_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_sync_renameA_ptr 0
#define __itt_sync_renameW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_sync_rename_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Is called when sync object is destroyed (needed to track lifetime of objects).
 */
void ITTAPI __itt_sync_destroy(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, sync_destroy, (void *addr))
#define __itt_sync_destroy     ITTNOTIFY_VOID(sync_destroy)
#define __itt_sync_destroy_ptr ITTNOTIFY_NAME(sync_destroy)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_sync_destroy(addr)
#define __itt_sync_destroy_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_sync_destroy_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/*****************************************************************//**
 * @name group of functions is used for performance measurement tools
 *********************************************************************/
/** @{ */
/**
 * @brief Enter spin loop on user-defined sync object.
 */
void ITTAPI __itt_sync_prepare(void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, sync_prepare, (void *addr))
#define __itt_sync_prepare     ITTNOTIFY_VOID(sync_prepare)
#define __itt_sync_prepare_ptr ITTNOTIFY_NAME(sync_prepare)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_sync_prepare(addr)
#define __itt_sync_prepare_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_sync_prepare_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Quit spin loop without acquiring spin object.
 */
void ITTAPI __itt_sync_cancel(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, sync_cancel, (void *addr))
#define __itt_sync_cancel     ITTNOTIFY_VOID(sync_cancel)
#define __itt_sync_cancel_ptr ITTNOTIFY_NAME(sync_cancel)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_sync_cancel(addr)
#define __itt_sync_cancel_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_sync_cancel_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Successful spin loop completion (sync object acquired).
 */
void ITTAPI __itt_sync_acquired(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, sync_acquired, (void *addr))
#define __itt_sync_acquired     ITTNOTIFY_VOID(sync_acquired)
#define __itt_sync_acquired_ptr ITTNOTIFY_NAME(sync_acquired)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_sync_acquired(addr)
#define __itt_sync_acquired_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_sync_acquired_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Start sync object releasing code. Is called before the lock release call.
 */
void ITTAPI __itt_sync_releasing(void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, sync_releasing, (void *addr))
#define __itt_sync_releasing     ITTNOTIFY_VOID(sync_releasing)
#define __itt_sync_releasing_ptr ITTNOTIFY_NAME(sync_releasing)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_sync_releasing(addr)
#define __itt_sync_releasing_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_sync_releasing_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} */

/**************************************************************//**
 * @name group of functions is used for correctness checking tools
 ******************************************************************/
/** @{ */
/**
 * @brief Fast synchronization which does no require spinning.
 * - This special function is to be used by TBB and OpenMP libraries only when they know
 *   there is no spin but they need to suppress TC warnings about shared variable modifications.
 * - It only has corresponding pointers in static library and does not have corresponding function
 *   in dynamic library.
 * @see void __itt_sync_prepare(void* addr);
 */
void ITTAPI __itt_fsync_prepare(void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, fsync_prepare, (void *addr))
#define __itt_fsync_prepare     ITTNOTIFY_VOID(fsync_prepare)
#define __itt_fsync_prepare_ptr ITTNOTIFY_NAME(fsync_prepare)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_fsync_prepare(addr)
#define __itt_fsync_prepare_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_fsync_prepare_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Fast synchronization which does no require spinning.
 * - This special function is to be used by TBB and OpenMP libraries only when they know
 *   there is no spin but they need to suppress TC warnings about shared variable modifications.
 * - It only has corresponding pointers in static library and does not have corresponding function
 *   in dynamic library.
 * @see void __itt_sync_cancel(void *addr);
 */
void ITTAPI __itt_fsync_cancel(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, fsync_cancel, (void *addr))
#define __itt_fsync_cancel     ITTNOTIFY_VOID(fsync_cancel)
#define __itt_fsync_cancel_ptr ITTNOTIFY_NAME(fsync_cancel)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_fsync_cancel(addr)
#define __itt_fsync_cancel_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_fsync_cancel_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Fast synchronization which does no require spinning.
 * - This special function is to be used by TBB and OpenMP libraries only when they know
 *   there is no spin but they need to suppress TC warnings about shared variable modifications.
 * - It only has corresponding pointers in static library and does not have corresponding function
 *   in dynamic library.
 * @see void __itt_sync_acquired(void *addr);
 */
void ITTAPI __itt_fsync_acquired(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, fsync_acquired, (void *addr))
#define __itt_fsync_acquired     ITTNOTIFY_VOID(fsync_acquired)
#define __itt_fsync_acquired_ptr ITTNOTIFY_NAME(fsync_acquired)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_fsync_acquired(addr)
#define __itt_fsync_acquired_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_fsync_acquired_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Fast synchronization which does no require spinning.
 * - This special function is to be used by TBB and OpenMP libraries only when they know
 *   there is no spin but they need to suppress TC warnings about shared variable modifications.
 * - It only has corresponding pointers in static library and does not have corresponding function
 *   in dynamic library.
 * @see void __itt_sync_releasing(void* addr);
 */
void ITTAPI __itt_fsync_releasing(void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, fsync_releasing, (void *addr))
#define __itt_fsync_releasing     ITTNOTIFY_VOID(fsync_releasing)
#define __itt_fsync_releasing_ptr ITTNOTIFY_NAME(fsync_releasing)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_fsync_releasing(addr)
#define __itt_fsync_releasing_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_fsync_releasing_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} */
/** @} sync group */

/**
 * @defgroup model Modeling by Intel(R) Parallel Advisor
 * @ingroup public
 * This is the subset of itt used for modeling by Intel(R) Parallel Advisor.
 * This API is called ONLY using annotate.h, by "Annotation" macros
 * the user places in their sources during the parallelism modeling steps.
 *
 * The requirements, constraints, design and implementation
 * for this interface are covered in:
 * Shared%20Documents/Design%20Documents/AdvisorAnnotations.doc
 *
 * site_begin/end and task_begin/end take the address of handle variables,
 * which are writeable by the API.  Handles must be 0 initialized prior
 * to the first call to begin, or may cause a run-time failure.
 * The handles are initialized in a multi-thread safe way by the API if
 * the handle is 0.  The commonly expected idiom is one static handle to
 * identify a site or task.  If a site or task of the same name has already
 * been started during this collection, the same handle MAY be returned,
 * but is not required to be - it is unspecified if data merging is done
 * based on name.  These routines also take an instance variable.  Like
 * the lexical instance, these must be 0 initialized.  Unlike the lexical
 * instance, this is used to track a single dynamic instance.
 *
 * API used by the Intel(R) Parallel Advisor to describe potential concurrency
 * and related activities. User-added source annotations expand to calls
 * to these procedures to enable modeling of a hypothetical concurrent
 * execution serially.
 * @{
 */
typedef void* __itt_model_site;             /*!< @brief handle for lexical site     */
typedef void* __itt_model_site_instance;    /*!< @brief handle for dynamic instance */
typedef void* __itt_model_task;             /*!< @brief handle for lexical site     */
typedef void* __itt_model_task_instance;    /*!< @brief handle for dynamic instance */

/**
 * @enum __itt_model_disable
 * @brief Enumerator for the disable methods.
 */
typedef enum {
    __itt_model_disable_observation,
    __itt_model_disable_collection
} __itt_model_disable;

/**
 * @brief ANNOTATE_SITE_BEGIN/ANNOTATE_SITE_END support.
 *
 * site_begin/end model a potential concurrency site.
 * site instances may be recursively nested with themselves.
 * site_end exits the most recently started but unended site for the current
 * thread.  The handle passed to end may be used to validate structure.
 * Instances of a site encountered on different threads concurrently
 * are considered completely distinct. If the site name for two different
 * lexical sites match, it is unspecified whether they are treated as the
 * same or different for data presentation.
 */
void ITTAPI __itt_model_site_begin(__itt_model_site *site, __itt_model_site_instance *instance, const char *name);
void ITTAPI __itt_model_site_end  (__itt_model_site *site, __itt_model_site_instance *instance);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_site_begin,  (__itt_model_site *site, __itt_model_site_instance *instance, const char *name))
ITT_STUBV(ITTAPI, void, model_site_end,    (__itt_model_site *site, __itt_model_site_instance *instance))
#define __itt_model_site_begin      ITTNOTIFY_VOID(model_site_begin)
#define __itt_model_site_begin_ptr  ITTNOTIFY_NAME(model_site_begin)
#define __itt_model_site_end        ITTNOTIFY_VOID(model_site_end)
#define __itt_model_site_end_ptr    ITTNOTIFY_NAME(model_site_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_site_begin(site, instance, name)
#define __itt_model_site_begin_ptr  0
#define __itt_model_site_end(site, instance)
#define __itt_model_site_end_ptr    0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_site_begin_ptr  0
#define __itt_model_site_end_ptr    0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_TASK_BEGIN/ANNOTATE_TASK_END support
 *
 * task_begin/end model a potential task, which is contained within the most
 * closely enclosing dynamic site.  task_end exits the most recently started
 * but unended task.  The handle passed to end may be used to validate
 * structure.  It is unspecified if bad dynamic nesting is detected.  If it
 * is, it should be encoded in the resulting data collection.  The collector
 * should not fail due to construct nesting issues, nor attempt to directly
 * indicate the problem.
 */
void ITTAPI __itt_model_task_begin(__itt_model_task *task, __itt_model_task_instance *instance, const char *name);
void ITTAPI __itt_model_task_end  (__itt_model_task *task, __itt_model_task_instance *instance);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_task_begin,  (__itt_model_task *task, __itt_model_task_instance *instance, const char *name))
ITT_STUBV(ITTAPI, void, model_task_end,    (__itt_model_task *task, __itt_model_task_instance *instance))
#define __itt_model_task_begin      ITTNOTIFY_VOID(model_task_begin)
#define __itt_model_task_begin_ptr  ITTNOTIFY_NAME(model_task_begin)
#define __itt_model_task_end        ITTNOTIFY_VOID(model_task_end)
#define __itt_model_task_end_ptr    ITTNOTIFY_NAME(model_task_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_task_begin(task, instance, name)
#define __itt_model_task_begin_ptr  0
#define __itt_model_task_end(task, instance)
#define __itt_model_task_end_ptr    0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_task_begin_ptr  0
#define __itt_model_task_end_ptr    0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_LOCK_ACQUIRE/ANNOTATE_LOCK_RELEASE support
 *
 * lock_acquire/release model a potential lock for both lockset and
 * performance modeling.  Each unique address is modeled as a separate
 * lock, with invalid addresses being valid lock IDs.  Specifically:
 * no storage is accessed by the API at the specified address - it is only
 * used for lock identification.  Lock acquires may be self-nested and are
 * unlocked by a corresponding number of releases.
 * (These closely correspond to __itt_sync_acquired/__itt_sync_releasing,
 * but may not have identical semantics.)
 */
void ITTAPI __itt_model_lock_acquire(void *lock);
void ITTAPI __itt_model_lock_release(void *lock);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_lock_acquire, (void *lock))
ITT_STUBV(ITTAPI, void, model_lock_release, (void *lock))
#define __itt_model_lock_acquire     ITTNOTIFY_VOID(model_lock_acquire)
#define __itt_model_lock_acquire_ptr ITTNOTIFY_NAME(model_lock_acquire)
#define __itt_model_lock_release     ITTNOTIFY_VOID(model_lock_release)
#define __itt_model_lock_release_ptr ITTNOTIFY_NAME(model_lock_release)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_lock_acquire(lock)
#define __itt_model_lock_acquire_ptr 0
#define __itt_model_lock_release(lock)
#define __itt_model_lock_release_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_lock_acquire_ptr 0
#define __itt_model_lock_release_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_RECORD_ALLOCATION/ANNOTATE_RECORD_DEALLOCATION support
 *
 * record_allocation/deallocation describe user-defined memory allocator
 * behavior, which may be required for correctness modeling to understand
 * when storage is not expected to be actually reused across threads.
 */
void ITTAPI __itt_model_record_allocation  (void *addr, size_t size);
void ITTAPI __itt_model_record_deallocation(void *addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_record_allocation,   (void *addr, size_t size))
ITT_STUBV(ITTAPI, void, model_record_deallocation, (void *addr))
#define __itt_model_record_allocation       ITTNOTIFY_VOID(model_record_allocation)
#define __itt_model_record_allocation_ptr   ITTNOTIFY_NAME(model_record_allocation)
#define __itt_model_record_deallocation     ITTNOTIFY_VOID(model_record_deallocation)
#define __itt_model_record_deallocation_ptr ITTNOTIFY_NAME(model_record_deallocation)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_record_allocation(addr, size)
#define __itt_model_record_allocation_ptr   0
#define __itt_model_record_deallocation(addr)
#define __itt_model_record_deallocation_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_record_allocation_ptr   0
#define __itt_model_record_deallocation_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_INDUCTION_USES support
 *
 * Note particular storage is inductive through the end of the current site
 */
void ITTAPI __itt_model_induction_uses(void* addr, size_t size);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_induction_uses, (void *addr, size_t size))
#define __itt_model_induction_uses     ITTNOTIFY_VOID(model_induction_uses)
#define __itt_model_induction_uses_ptr ITTNOTIFY_NAME(model_induction_uses)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_induction_uses(addr, size)
#define __itt_model_induction_uses_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_induction_uses_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_REDUCTION_USES support
 *
 * Note particular storage is used for reduction through the end
 * of the current site
 */
void ITTAPI __itt_model_reduction_uses(void* addr, size_t size);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_reduction_uses, (void *addr, size_t size))
#define __itt_model_reduction_uses     ITTNOTIFY_VOID(model_reduction_uses)
#define __itt_model_reduction_uses_ptr ITTNOTIFY_NAME(model_reduction_uses)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_reduction_uses(addr, size)
#define __itt_model_reduction_uses_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_reduction_uses_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_OBSERVE_USES support
 *
 * Have correctness modeling record observations about uses of storage
 * through the end of the current site
 */
void ITTAPI __itt_model_observe_uses(void* addr, size_t size);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_observe_uses, (void *addr, size_t size))
#define __itt_model_observe_uses     ITTNOTIFY_VOID(model_observe_uses)
#define __itt_model_observe_uses_ptr ITTNOTIFY_NAME(model_observe_uses)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_observe_uses(addr, size)
#define __itt_model_observe_uses_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_observe_uses_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_CLEAR_USES support
 *
 * Clear the special handling of a piece of storage related to induction,
 * reduction or observe_uses
 */
void ITTAPI __itt_model_clear_uses(void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_clear_uses, (void *addr))
#define __itt_model_clear_uses     ITTNOTIFY_VOID(model_clear_uses)
#define __itt_model_clear_uses_ptr ITTNOTIFY_NAME(model_clear_uses)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_clear_uses(addr)
#define __itt_model_clear_uses_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_clear_uses_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief ANNOTATE_DISABLE_*_PUSH/ANNOTATE_DISABLE_*_POP support
 *
 * disable_push/disable_pop push and pop disabling based on a parameter.
 * Disabling observations stops processing of memory references during
 * correctness modeling, and all annotations that occur in the disabled
 * region.  This allows description of code that is expected to be handled
 * specially during conversion to parallelism or that is not recognized
 * by tools (e.g. some kinds of synchronization operations.)
 * This mechanism causes all annotations in the disabled region, other
 * than disable_push and disable_pop, to be ignored.  (For example, this
 * might validly be used to disable an entire parallel site and the contained
 * tasks and locking in it for data collection purposes.)
 * The disable for collection is a more expensive operation, but reduces
 * collector overhead significantly.  This applies to BOTH correctness data
 * collection and performance data collection.  For example, a site
 * containing a task might only enable data collection for the first 10
 * iterations.  Both performance and correctness data should reflect this,
 * and the program should run as close to full speed as possible when
 * collection is disabled.
 */
void ITTAPI __itt_model_disable_push(__itt_model_disable x);
void ITTAPI __itt_model_disable_pop(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, model_disable_push, (__itt_model_disable x))
ITT_STUBV(ITTAPI, void, model_disable_pop,  (void))
#define __itt_model_disable_push     ITTNOTIFY_VOID(model_disable_push)
#define __itt_model_disable_push_ptr ITTNOTIFY_NAME(model_disable_push)
#define __itt_model_disable_pop      ITTNOTIFY_VOID(model_disable_pop)
#define __itt_model_disable_pop_ptr  ITTNOTIFY_NAME(model_disable_pop)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_model_disable_push(x)
#define __itt_model_disable_push_ptr 0
#define __itt_model_disable_pop()
#define __itt_model_disable_pop_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_model_disable_push_ptr 0
#define __itt_model_disable_pop_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} model group */

/**
 * @defgroup heap Heap
 * @ingroup public
 * Heap group
 * @{
 */

typedef void* __itt_heap_function;

/**
 * @brief Create an identification for heap function.
 * @return non-zero identifier or NULL
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
__itt_heap_function ITTAPI __itt_heap_function_createA(const char*    name, const char*    domain);
__itt_heap_function ITTAPI __itt_heap_function_createW(const wchar_t* name, const wchar_t* domain);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_heap_function_create     __itt_heap_function_createW
#  define __itt_heap_function_create_ptr __itt_heap_function_createW_ptr
#else
#  define __itt_heap_function_create     __itt_heap_function_createA
#  define __itt_heap_function_create_ptr __itt_heap_function_createA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
__itt_heap_function ITTAPI __itt_heap_function_create(const char* name, const char* domain);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUB(ITTAPI, __itt_heap_function, heap_function_createA, (const char*    name, const char*    domain))
ITT_STUB(ITTAPI, __itt_heap_function, heap_function_createW, (const wchar_t* name, const wchar_t* domain))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB(ITTAPI, __itt_heap_function, heap_function_create,  (const char*    name, const char*    domain))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_heap_function_createA     ITTNOTIFY_DATA(heap_function_createA)
#define __itt_heap_function_createA_ptr ITTNOTIFY_NAME(heap_function_createA)
#define __itt_heap_function_createW     ITTNOTIFY_DATA(heap_function_createW)
#define __itt_heap_function_createW_ptr ITTNOTIFY_NAME(heap_function_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_heap_function_create      ITTNOTIFY_DATA(heap_function_create)
#define __itt_heap_function_create_ptr  ITTNOTIFY_NAME(heap_function_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_heap_function_createA(name, domain) (__itt_heap_function)0
#define __itt_heap_function_createA_ptr 0
#define __itt_heap_function_createW(name, domain) (__itt_heap_function)0
#define __itt_heap_function_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_heap_function_create(name, domain)  (__itt_heap_function)0
#define __itt_heap_function_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_heap_function_createA_ptr 0
#define __itt_heap_function_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_heap_function_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an allocation begin occurrence.
 */
void ITTAPI __itt_heap_allocate_begin(__itt_heap_function h, size_t size, int initialized);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_allocate_begin, (__itt_heap_function h, size_t size, int initialized))
#define __itt_heap_allocate_begin     ITTNOTIFY_VOID(heap_allocate_begin)
#define __itt_heap_allocate_begin_ptr ITTNOTIFY_NAME(heap_allocate_begin)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_allocate_begin(h, size, initialized)
#define __itt_heap_allocate_begin_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_allocate_begin_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an allocation end occurrence.
 */
void ITTAPI __itt_heap_allocate_end(__itt_heap_function h, void** addr, size_t size, int initialized);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_allocate_end, (__itt_heap_function h, void** addr, size_t size, int initialized))
#define __itt_heap_allocate_end     ITTNOTIFY_VOID(heap_allocate_end)
#define __itt_heap_allocate_end_ptr ITTNOTIFY_NAME(heap_allocate_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_allocate_end(h, addr, size, initialized)
#define __itt_heap_allocate_end_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_allocate_end_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an free begin occurrence.
 */
void ITTAPI __itt_heap_free_begin(__itt_heap_function h, void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_free_begin, (__itt_heap_function h, void* addr))
#define __itt_heap_free_begin     ITTNOTIFY_VOID(heap_free_begin)
#define __itt_heap_free_begin_ptr ITTNOTIFY_NAME(heap_free_begin)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_free_begin(h, addr)
#define __itt_heap_free_begin_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_free_begin_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an free end occurrence.
 */
void ITTAPI __itt_heap_free_end(__itt_heap_function h, void* addr);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_free_end, (__itt_heap_function h, void* addr))
#define __itt_heap_free_end     ITTNOTIFY_VOID(heap_free_end)
#define __itt_heap_free_end_ptr ITTNOTIFY_NAME(heap_free_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_free_end(h, addr)
#define __itt_heap_free_end_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_free_end_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an reallocation begin occurrence.
 */
void ITTAPI __itt_heap_reallocate_begin(__itt_heap_function h, void* addr, size_t new_size, int initialized);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_reallocate_begin, (__itt_heap_function h, void* addr, size_t new_size, int initialized))
#define __itt_heap_reallocate_begin     ITTNOTIFY_VOID(heap_reallocate_begin)
#define __itt_heap_reallocate_begin_ptr ITTNOTIFY_NAME(heap_reallocate_begin)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_reallocate_begin(h, addr, new_size, initialized)
#define __itt_heap_reallocate_begin_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_reallocate_begin_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an reallocation end occurrence.
 */
void ITTAPI __itt_heap_reallocate_end(__itt_heap_function h, void* addr, void** new_addr, size_t new_size, int initialized);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_reallocate_end, (__itt_heap_function h, void* addr, void** new_addr, size_t new_size, int initialized))
#define __itt_heap_reallocate_end     ITTNOTIFY_VOID(heap_reallocate_end)
#define __itt_heap_reallocate_end_ptr ITTNOTIFY_NAME(heap_reallocate_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_reallocate_end(h, addr, new_addr, new_size, initialized)
#define __itt_heap_reallocate_end_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_reallocate_end_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @brief internal access begin */
void ITTAPI __itt_heap_internal_access_begin(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_internal_access_begin,  (void))
#define __itt_heap_internal_access_begin      ITTNOTIFY_VOID(heap_internal_access_begin)
#define __itt_heap_internal_access_begin_ptr  ITTNOTIFY_NAME(heap_internal_access_begin)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_internal_access_begin()
#define __itt_heap_internal_access_begin_ptr  0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_internal_access_begin_ptr  0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @brief internal access end */
void ITTAPI __itt_heap_internal_access_end(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, heap_internal_access_end, (void))
#define __itt_heap_internal_access_end     ITTNOTIFY_VOID(heap_internal_access_end)
#define __itt_heap_internal_access_end_ptr ITTNOTIFY_NAME(heap_internal_access_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_heap_internal_access_end()
#define __itt_heap_internal_access_end_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_heap_internal_access_end_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} heap group */
/** @endcond */
/* ========================================================================== */

/** @cond exclude_from_documentation */
typedef struct ___itt_domain
{
    volatile int flags; /*!< Zero if disabled, non-zero if enabled. The meaning of different non-zero values is reserved to the runtime */
    const char* nameA;  /*!< Copy of original name in ASCII. */
#if defined(UNICODE) || defined(_UNICODE)
    const wchar_t* nameW; /*!< Copy of original name in UNICODE. */
#else  /* UNICODE || _UNICODE */
    void* nameW;
#endif /* UNICODE || _UNICODE */
    int   extra1; /*!< Reserved to the runtime */
    void* extra2; /*!< Reserved to the runtime */
    struct ___itt_domain* next;
} __itt_domain;
/** @endcond */

/**
 * @ingroup domains
 * @brief Create a domain.
 * Create domain using some domain name: the URI naming style is recommended.
 * Because the set of domains is expected to be static over the application's execution time, there is no mechanism to 
 * destroy a domain.
 * Any domain can be accessed by any thread in the process, regardless of which thread created
 * the domain. This call is thread-safe.
 * @param[in] name Name of domain
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
__itt_domain* ITTAPI __itt_domain_createA(const char    *name);
__itt_domain* ITTAPI __itt_domain_createW(const wchar_t *name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_domain_create				__itt_domain_createW
#  define __itt_domain_create_ptr			__itt_domain_createW_ptr
#else /* UNICODE */
#  define __itt_domain_create		    __itt_domain_createA
#  define __itt_domain_create_ptr		__itt_domain_createA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
__itt_domain* ITTAPI __itt_domain_create(const char *name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUB(ITTAPI, __itt_domain*, domain_createA, (const char    *name))
ITT_STUB(ITTAPI, __itt_domain*, domain_createW, (const wchar_t *name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB(ITTAPI, __itt_domain*, domain_create,  (const char    *name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_domain_createA     ITTNOTIFY_DATA(domain_createA)
#define __itt_domain_createA_ptr ITTNOTIFY_NAME(domain_createA)
#define __itt_domain_createW     ITTNOTIFY_DATA(domain_createW)
#define __itt_domain_createW_ptr ITTNOTIFY_NAME(domain_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_domain_create     ITTNOTIFY_DATA(domain_create)
#define __itt_domain_create_ptr ITTNOTIFY_NAME(domain_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_domain_createA(name) (__itt_domain*)0
#define __itt_domain_createA_ptr 0
#define __itt_domain_createW(name) (__itt_domain*)0
#define __itt_domain_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_domain_create(name)  (__itt_domain*)0
#define __itt_domain_create_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_domain_createA_ptr 0
#define __itt_domain_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_domain_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_documentation */
typedef struct ___itt_clock_info
{
    unsigned long long clock_freq; /*!< Clock domain frequency */
    unsigned long long clock_base; /*!< Clock domain base timestamp */
} __itt_clock_info;
/** @endcond */

/** @cond exclude_from_documentation */
typedef void (ITTAPI *__itt_get_clock_info_fn)(__itt_clock_info* clock_info, void* data);
/** @endcond */

/** @cond exclude_from_documentation */
typedef struct ___itt_clock_domain
{
    __itt_clock_info info; /*!< Most recent clock domain info */
    __itt_get_clock_info_fn fn; /*!< Callback function pointer */
    void* fn_data; /*!< Input argument for the callback function */
    int   extra1; /*!< Reserved. Mast be zero   */
    void* extra2; /*!< Reserved. Mast be zero   */
    struct ___itt_clock_domain* next;
} __itt_clock_domain;
/** @endcond */

/**
 * @ingroup clockdomains
 * @brief Create a clock domain.
 * Certain applications require the capability to trace their application using
 * a clock domain different than the CPU, for instance the instrumentation of events
 * that occur on a GPU.
 * Because the set of domains is expected to be static over the application's execution time,
 * there is no mechanism to destroy a domain.
 * Any domain can be accessed by any thread in the process, regardless of which thread created
 * the domain. This call is thread-safe.
 * @param[in] fn A pointer to a callback function which retrieves alternative CPU timestamps
 * @param[in] fn_data Argument for a callback function; may be NULL
 */
__itt_clock_domain* ITTAPI __itt_clock_domain_create(__itt_get_clock_info_fn fn, void* fn_data);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUB(ITTAPI, __itt_clock_domain*, clock_domain_create, (__itt_get_clock_info_fn fn, void* fn_data))
#define __itt_clock_domain_create			ITTNOTIFY_DATA(clock_domain_create)
#define __itt_clock_domain_create_ptr		ITTNOTIFY_NAME(clock_domain_create)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_clock_domain_create(fn,fn_data) (__itt_clock_domain*)0
#define __itt_clock_domain_create_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_clock_domain_create_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup clockdomains
 * @brief Recalculate clock domains frequences and clock base timestamps.
 */
void ITTAPI __itt_clock_domain_reset(void);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, clock_domain_reset, (void))
#define __itt_clock_domain_reset     ITTNOTIFY_VOID(clock_domain_reset)
#define __itt_clock_domain_reset_ptr ITTNOTIFY_NAME(clock_domain_reset)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_clock_domain_reset()
#define __itt_clock_domain_reset_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_clock_domain_reset_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_documentation */
typedef struct ___itt_id
{
    unsigned long long d1, d2, d3;
} __itt_id;
/** @endcond */

static const __itt_id __itt_null = { 0, 0, 0 };

/**
 * @ingroup ids
 * @brief This is a convenience function to initialize an __itt_id structure. This function
 * does not affect the trace collector runtime in any way. After you make the ID with this
 * function, you still must create it with the __itt_id_create function before using the ID
 * to identify a named entity.
 * @param[in] addr The address of object; high QWORD of the ID value.
 * @param[in] extra The extra data to unique identify object; low QWORD of the ID value.
 */

INLINE __itt_id ITTAPI __itt_id_make(void* addr, unsigned long long extra)
{
    __itt_id id;
    id.d1 = (unsigned long long)((size_t)addr);
    id.d2 = (unsigned long long)extra;
    id.d3 = (unsigned long long)0; /* Reserved. Mast be zero */
    return id;
}

/**
 * @ingroup ids
 * @brief Create an instance of identifier. This establishes the beginning of the lifetime of 
 * an instance of the given ID in the trace. Once this lifetime starts, the ID can be used to
 * tag named entity instances in calls such as __itt_task_begin, and to specify relationships among
 * identified named entity instances, using the \ref relations APIs.
 * @param[in] domain The domain controlling the execution of this call.
 * @param[in] id The ID to create.
 */
void ITTAPI __itt_id_create(const __itt_domain *domain, __itt_id id);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, id_create, (const __itt_domain *domain, __itt_id id))
#define __itt_id_create(domain, id) ITTNOTIFY_VOID_D1(id_create,domain,id)
#define __itt_id_create_ptr  ITTNOTIFY_NAME(id_create)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_id_create(domain,id)
#define __itt_id_create_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_id_create_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup ids
 * @brief Destroy an instance of identifier. This ends the lifetime of the current instance of the
 * given ID value in the trace. Any relationships that are established after this lifetime ends are
 * invalid. This call must be performed before the given ID value can be reused for a different 
 * named entity instance.
 * @param[in] domain The domain controlling the execution of this call.
 * @param[in] id The ID to destroy.
 */
void ITTAPI __itt_id_destroy(const __itt_domain *domain, __itt_id id);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, id_destroy, (const __itt_domain *domain, __itt_id id))
#define __itt_id_destroy(domain, id) ITTNOTIFY_VOID_D1(id_destroy,domain,id)
#define __itt_id_destroy_ptr  ITTNOTIFY_NAME(id_destroy)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_id_destroy(domain,id)
#define __itt_id_destroy_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_id_destroy_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup clockdomain
 * @brief Create an instance of identifier. This establishes the beginning of the lifetime of 
 * an instance of the given ID in the trace. Once this lifetime starts, the ID can be used to
 * tag named entity instances in calls such as __itt_task_begin, and to specify relationships among
 * identified named entity instances, using the \ref relations APIs.
 * @param[in] domain The domain controlling the execution of this call.
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] id The ID to create.
 */
void ITTAPI __itt_id_create_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id);

/**
 * @ingroup clockdomain
 * @brief Destroy an instance of identifier. This ends the lifetime of the current instance of the
 * given ID value in the trace. Any relationships that are established after this lifetime ends are
 * invalid. This call must be performed before the given ID value can be reused for a different 
 * named entity instance.
 * @param[in] domain The domain controlling the execution of this call.
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] id The ID to destroy.
 */
void ITTAPI __itt_id_destroy_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, id_create_ex,  (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id))
ITT_STUBV(ITTAPI, void, id_destroy_ex, (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id))
#define __itt_id_create_ex(domain, clock_domain, timestamp, id)  ITTNOTIFY_VOID_D3(id_create_ex,domain,clock_domain,timestamp,id)
#define __itt_id_create_ex_ptr       ITTNOTIFY_NAME(id_create_ex)
#define __itt_id_destroy_ex(domain, clock_domain, timestamp, id) ITTNOTIFY_VOID_D3(id_destroy_ex,domain,clock_domain,timestamp,id)
#define __itt_id_destroy_ex_ptr      ITTNOTIFY_NAME(id_destroy_ex)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_id_create_ex(domain,clock_domain,timestamp,id)
#define __itt_id_create_ex_ptr    0
#define __itt_id_destroy_ex(domain,clock_domain,timestamp,id)
#define __itt_id_destroy_ex_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_id_create_ex_ptr    0
#define __itt_id_destroy_ex_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_documentation */
typedef struct ___itt_string_handle
{
    const char* strA; /*!< Copy of original string in ASCII. */
#if defined(UNICODE) || defined(_UNICODE)
    const wchar_t* strW; /*!< Copy of original string in UNICODE. */
#else  /* UNICODE || _UNICODE */
    void* strW;
#endif /* UNICODE || _UNICODE */
    int   extra1; /*!< Reserved. Mast be zero   */
    void* extra2; /*!< Reserved. Mast be zero   */
    struct ___itt_string_handle* next;
} __itt_string_handle;
/** @endcond */

/**
 * @ingroup handles
 * @brief Create a string handle.
 * Create and return handle value that can be associated with a string.
 * Consecutive calls to __itt_string_handle_create with the same name
 * return the same value. Because the set of string handles is expected to remain
 * static during the application's execution time, there is no mechanism to destroy a string handle.
 * Any string handle can be accessed by any thread in the process, regardless of which thread created
 * the string handle. This call is thread-safe.
 * @param[in] name The input string
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
__itt_string_handle* ITTAPI __itt_string_handle_createA(const char    *name);
__itt_string_handle* ITTAPI __itt_string_handle_createW(const wchar_t *name);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_string_handle_create			__itt_string_handle_createW
#  define __itt_string_handle_create_ptr		__itt_string_handle_createW_ptr
#else /* UNICODE */
#  define __itt_string_handle_create			__itt_string_handle_createA
#  define __itt_string_handle_create_ptr		__itt_string_handle_createA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
__itt_string_handle* ITTAPI __itt_string_handle_create(const char *name);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUB(ITTAPI, __itt_string_handle*, string_handle_createA, (const char    *name))
ITT_STUB(ITTAPI, __itt_string_handle*, string_handle_createW, (const wchar_t *name))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB(ITTAPI, __itt_string_handle*, string_handle_create,  (const char    *name))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_string_handle_createA     ITTNOTIFY_DATA(string_handle_createA)
#define __itt_string_handle_createA_ptr ITTNOTIFY_NAME(string_handle_createA)
#define __itt_string_handle_createW     ITTNOTIFY_DATA(string_handle_createW)
#define __itt_string_handle_createW_ptr ITTNOTIFY_NAME(string_handle_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_string_handle_create     ITTNOTIFY_DATA(string_handle_create)
#define __itt_string_handle_create_ptr ITTNOTIFY_NAME(string_handle_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_string_handle_createA(name) (__itt_string_handle*)0
#define __itt_string_handle_createA_ptr 0
#define __itt_string_handle_createW(name) (__itt_string_handle*)0
#define __itt_string_handle_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_string_handle_create(name)  (__itt_string_handle*)0
#define __itt_string_handle_create_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_string_handle_createA_ptr 0
#define __itt_string_handle_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_string_handle_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_gpa_documentation */
/**
 * @ingroup regions
 * @brief Begin of region instance.
 * Successive calls to __itt_region_begin with the same ID are ignored
 * until a call to __itt_region_end with the same ID
 * @param[in] domain The domain for this region instance
 * @param[in] id The instance ID for this region instance. Must not be __itt_null
 * @param[in] parentid The instance ID for the parent of this region instance, or __itt_null
 * @param[in] name The name of this region
 */
void ITTAPI __itt_region_begin(const __itt_domain *domain, __itt_id id, __itt_id parentid, __itt_string_handle *name);

/**
 * @ingroup regions
 * @brief End of region instance.
 * The first call to __itt_region_end with a given ID ends the
 * region. Successive calls with the same ID are ignored, as are
 * calls that do not have a matching __itt_region_begin call.
 * @param[in] domain The domain for this region instance
 * @param[in] id The instance ID for this region instance
 */
void ITTAPI __itt_region_end(const __itt_domain *domain, __itt_id id);
/** @endcond */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, region_begin, (const __itt_domain *domain, __itt_id id, __itt_id parentid, __itt_string_handle *name))
ITT_STUBV(ITTAPI, void, region_end,   (const __itt_domain *domain, __itt_id id))
#define __itt_region_begin(domain, id, parentid, name)  ITTNOTIFY_VOID_D3(region_begin,domain,id,parentid,name)
#define __itt_region_begin_ptr							ITTNOTIFY_NAME(region_begin)
#define __itt_region_end(domain, id)					ITTNOTIFY_VOID_D1(region_end,domain,id)
#define __itt_region_end_ptr							ITTNOTIFY_NAME(region_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_region_begin(domain, id, parentid, name)
#define __itt_region_begin_ptr 0
#define __itt_region_end(domain, id)
#define __itt_region_end_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_region_begin_ptr 0
#define __itt_region_end_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_gpa_documentation */
/**
 * @ingroup frames
 * @brief Begin a frame instance.
 * Successive calls to __itt_frame_begin with the
 * same ID are ignored until a call to __itt_frame_end with the same ID.
 * @param[in] domain The domain for this frame instance
 * @param[in] id The instance ID for this frame instance or NULL
 */
void ITTAPI __itt_frame_begin_v3(const __itt_domain *domain, __itt_id *id);

/**
 * @ingroup frames
 * @brief End a frame instance.
 * The first call to __itt_frame_end with a given ID
 * ends the frame. Successive calls with the same ID are ignored, as are
 * calls that do not have a matching __itt_frame_begin call.
 * @param[in] domain The domain for this frame instance
 * @param[in] id The instance ID for this frame instance or NULL for current
 */
void ITTAPI __itt_frame_end_v3(const __itt_domain *domain, __itt_id *id);
/** @endcond */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, frame_begin_v3, (const __itt_domain *domain, __itt_id *id))
ITT_STUBV(ITTAPI, void, frame_end_v3,   (const __itt_domain *domain, __itt_id *id))
#define __itt_frame_begin_v3(d,x)   ITTNOTIFY_VOID_D1(frame_begin_v3,d,x)
#define __itt_frame_begin_v3_ptr    ITTNOTIFY_NAME(frame_begin_v3)
#define __itt_frame_end_v3(d,x)     ITTNOTIFY_VOID_D1(frame_end_v3,d,x)
#define __itt_frame_end_v3_ptr      ITTNOTIFY_NAME(frame_end_v3)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_frame_begin_v3(domain,id)
#define __itt_frame_begin_v3_ptr 0
#define __itt_frame_end_v3(domain,id)
#define __itt_frame_end_v3_ptr   0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_frame_begin_v3_ptr 0
#define __itt_frame_end_v3_ptr   0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup task_groups
 * @brief Denotes a task_group instance.
 * Successive calls to __itt_task_group with the same ID are ignored.
 * @param[in] domain The domain for this task_group instance
 * @param[in] id The instance ID for this task_group instance. Must not be __itt_null.
 * @param[in] parentid The instance ID for the parent of this task_group instance, or __itt_null.
 * @param[in] name The name of this task_group
 */
void ITTAPI __itt_task_group(const __itt_domain *domain, __itt_id id, __itt_id parentid, __itt_string_handle *name);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, task_group, (const __itt_domain *domain, __itt_id id, __itt_id parentid, __itt_string_handle *name))
#define __itt_task_group(domain,id,parentid,name) 	ITTNOTIFY_VOID_D3(task_group,domain,id,parentid,name)
#define __itt_task_group_ptr      					ITTNOTIFY_NAME(task_group)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_task_group(domain,id,parentid,name)
#define __itt_task_group_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_task_group_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup tasks
 * @brief Begin a task instance.
 * @param[in] domain The domain for this task
 * @param[in] taskid The instance ID for this task instance, or __itt_null
 * @param[in] parentid The parent instance to which this task instance belongs, or __itt_null
 * @param[in] name The name of this task
 */
void ITTAPI __itt_task_begin(const __itt_domain *domain, __itt_id taskid, __itt_id parentid, __itt_string_handle *name);

/**
 * @ingroup tasks
 * @brief Begin a task instance.
 * @param[in] domain The domain for this task
 * @param[in] taskid The identifier for this task instance, or __itt_null
 * @param[in] parentid The parent of this task, or __itt_null
 * @param[in] fn The pointer to the function you are tracing
 */
void ITTAPI __itt_task_begin_fn(const __itt_domain *domain, __itt_id taskid, __itt_id parentid, void* fn);

/**
 * @ingroup tasks
 * @brief End the current task instance.
 * @param[in] domain The domain for this task
 */
void ITTAPI __itt_task_end(const __itt_domain *domain);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, task_begin,          (const __itt_domain *domain, __itt_id id, __itt_id parentid, __itt_string_handle *name))
ITT_STUBV(ITTAPI, void, task_begin_fn,       (const __itt_domain *domain, __itt_id id, __itt_id parentid, void* fn))
ITT_STUBV(ITTAPI, void, task_end,            (const __itt_domain *domain))
#define __itt_task_begin(domain, id, parentid, name)		ITTNOTIFY_VOID_D3(task_begin,domain,id,parentid,name)
#define __itt_task_begin_ptr								ITTNOTIFY_NAME(task_begin)
#define __itt_task_begin_fn(domain, id, parentid, fn)		ITTNOTIFY_VOID_D3(task_begin_fn,domain,id,parentid,fn)
#define __itt_task_begin_fn_ptr								ITTNOTIFY_NAME(task_begin_fn)
#define __itt_task_end(domain)								ITTNOTIFY_VOID_D0(task_end,domain)
#define __itt_task_end_ptr									ITTNOTIFY_NAME(task_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_task_begin(domain,id,parentid,name)
#define __itt_task_begin_ptr          0
#define __itt_task_begin_fn(domain,id,parentid,fn)
#define __itt_task_begin_fn_ptr       0
#define __itt_task_end(domain)
#define __itt_task_end_ptr            0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_task_begin_ptr          0
#define __itt_task_begin_fn_ptr       0
#define __itt_task_end_ptr            0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup clockdomain
 * @brief Begin a task instance.
 * @param[in] domain The domain for this task
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] taskid The instance ID for this task instance, or __itt_null
 * @param[in] parentid The parent instance to which this task instance belongs, or __itt_null
 * @param[in] name The name of this task
 */
void ITTAPI __itt_task_begin_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid, __itt_id parentid, __itt_string_handle* name);

/**
 * @ingroup clockdomain
 * @brief Begin a task instance.
 * @param[in] domain The domain for this task
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] taskid The identifier for this task instance, or __itt_null
 * @param[in] parentid The parent of this task, or __itt_null
 * @param[in] fn The pointer to the function you are tracing
 */
void ITTAPI __itt_task_begin_fn_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid, __itt_id parentid, void* fn);

/**
 * @ingroup clockdomain
 * @brief End the current task instance.
 * @param[in] domain The domain for this task
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 */
void ITTAPI __itt_task_end_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, task_begin_ex,          (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id, __itt_id parentid, __itt_string_handle *name))
ITT_STUBV(ITTAPI, void, task_begin_fn_ex,       (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id, __itt_id parentid, void* fn))
ITT_STUBV(ITTAPI, void, task_end_ex,            (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp))
#define __itt_task_begin_ex(domain, clock_domain, timestamp, id, parentid, name)      ITTNOTIFY_VOID_D5(task_begin_ex,domain,clock_domain,timestamp,id,parentid,name)
#define __itt_task_begin_ex_ptr														  ITTNOTIFY_NAME(task_begin_ex)
#define __itt_task_begin_fn_ex(domain, clock_domain, timestamp, id, parentid, fn)     ITTNOTIFY_VOID_D5(task_begin_fn_ex,domain,clock_domain,timestamp,id,parentid,fn)
#define __itt_task_begin_fn_ex_ptr													  ITTNOTIFY_NAME(task_begin_fn_ex)
#define __itt_task_end_ex(domain, clock_domain, timestamp)							  ITTNOTIFY_VOID_D2(task_end_ex, domain, clock_domain, timestamp)
#define __itt_task_end_ex_ptr														  ITTNOTIFY_NAME(task_end_ex)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_task_begin_ex(domain,clock_domain,timestamp,id,parentid,name)
#define __itt_task_begin_ex_ptr          0
#define __itt_task_begin_fn_ex(domain,clock_domain,timestamp,id,parentid,fn)
#define __itt_task_begin_fn_ex_ptr       0
#define __itt_task_end_ex(domain,clock_domain,timestamp)
#define __itt_task_end_ex_ptr            0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_task_begin_ex_ptr          0
#define __itt_task_begin_fn_ex_ptr       0
#define __itt_task_end_ex_ptr            0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

#ifdef INTEL_ITTNOTIFY_API_PRIVATE
/**
 * @ingroup tasks
 * @brief Begin an overlapped task instance.
 * @param[in] domain The domain for this task.
 * @param[in] taskid The identifier for this task instance, *cannot* be __itt_null.
 * @param[in] parentid The parent of this task, or __itt_null.
 * @param[in] name The name of this task.
 */
void ITTAPI __itt_task_begin_overlapped(const __itt_domain* domain, __itt_id taskid, __itt_id parentid, __itt_string_handle* name);

/**
 * @ingroup clockdomain
 * @brief Begin an overlapped task instance.
 * @param[in] domain The domain for this task
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] taskid The identifier for this task instance, *cannot* be __itt_null.
 * @param[in] parentid The parent of this task, or __itt_null.
 * @param[in] name The name of this task.
 */
void ITTAPI __itt_task_begin_overlapped_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid, __itt_id parentid, __itt_string_handle* name);

/**
 * @ingroup tasks
 * @brief End an overlapped task instance.
 * @param[in] domain The domain for this task
 * @param[in] taskid Explicit ID of finished task
 */
void ITTAPI __itt_task_end_overlapped(const __itt_domain *domain, __itt_id taskid);

/**
 * @ingroup clockdomain
 * @brief End an overlapped task instance.
 * @param[in] domain The domain for this task
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] taskid Explicit ID of finished task
 */
void ITTAPI __itt_task_end_overlapped_ex(const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, task_begin_overlapped,          (const __itt_domain *domain, __itt_id taskid, __itt_id parentid, __itt_string_handle *name))
ITT_STUBV(ITTAPI, void, task_begin_overlapped_ex,       (const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid, __itt_id parentid, __itt_string_handle* name))
ITT_STUBV(ITTAPI, void, task_end_overlapped,            (const __itt_domain *domain, __itt_id taskid))
ITT_STUBV(ITTAPI, void, task_end_overlapped_ex,         (const __itt_domain* domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id taskid))
#define __itt_task_begin_overlapped(domain, id, parentid, name)									ITTNOTIFY_VOID_D3(task_begin_overlapped,domain,id,parentid,name)
#define __itt_task_begin_overlapped_ptr															ITTNOTIFY_NAME(task_begin_overlapped)
#define __itt_task_begin_overlapped_ex(domain, clock_domain, timestamp, id, parentid, name)     ITTNOTIFY_VOID_D5(task_begin_overlapped_ex,domain,clock_domain,timestamp,id,parentid,name)
#define __itt_task_begin_overlapped_ex_ptr														ITTNOTIFY_NAME(task_begin_overlapped_ex)
#define __itt_task_end_overlapped(domain, id)													ITTNOTIFY_VOID_D1(task_end_overlapped,domain,id)
#define __itt_task_end_overlapped_ptr															ITTNOTIFY_NAME(task_end_overlapped)
#define __itt_task_end_overlapped_ex(domain, clock_domain, timestamp, id)						ITTNOTIFY_VOID_D3(task_end_overlapped_ex,domain,clock_domain,timestamp,id)
#define __itt_task_end_overlapped_ex_ptr														ITTNOTIFY_NAME(task_end_overlapped_ex)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_task_begin_overlapped(domain,taskid,parentid,name)
#define __itt_task_begin_overlapped_ptr         0
#define __itt_task_begin_overlapped_ex(domain,clock_domain,timestamp,taskid,parentid,name)
#define __itt_task_begin_overlapped_ex_ptr      0
#define __itt_task_end_overlapped(domain,taskid)
#define __itt_task_end_overlapped_ptr           0
#define __itt_task_end_overlapped_ex(domain,clock_domain,timestamp,taskid)
#define __itt_task_end_overlapped_ex_ptr        0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_task_begin_overlapped_ptr         0
#define __itt_task_begin_overlapped_ex_ptr      0
#define __itt_task_end_overlapped_ptr           0
#define __itt_task_end_overlapped_ex_ptr        0
#endif /* INTEL_NO_MACRO_BODY */
#endif /* INTEL_ITTNOTIFY_API_PRIVATE */
/** @endcond */

/**
 * @ingroup counters
 * @brief Increment a counter by one.
 * @param[in] domain The domain controlling the call. 
 * @param[in] name The name of the counter
 */
void ITTAPI __itt_counter_inc_v3(const __itt_domain *domain, __itt_string_handle *name);

/**
 * @ingroup counters
 * @brief Increment a counter by a specific amount.
 * @param[in] domain The domain controlling the call 
 * @param[in] name The name of the counter
 * @param[in] delta The amount by which to increment the counter
 */
void ITTAPI __itt_counter_inc_delta_v3(const __itt_domain *domain, __itt_string_handle *name, unsigned long long delta);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, counter_inc_v3,       (const __itt_domain *domain, __itt_string_handle *name))
ITT_STUBV(ITTAPI, void, counter_inc_delta_v3, (const __itt_domain *domain, __itt_string_handle *name, unsigned long long delta))
#define __itt_counter_inc_v3(domain, name)				ITTNOTIFY_VOID_D1(counter_inc_v3,domain,name)
#define __itt_counter_inc_v3_ptr						ITTNOTIFY_NAME(counter_inc_v3)
#define __itt_counter_inc_delta_v3(domain, name, delta) ITTNOTIFY_VOID_D2(counter_inc_delta_v3,domain,name,delta)
#define __itt_counter_inc_delta_v3_ptr					ITTNOTIFY_NAME(counter_inc_delta_v3)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_counter_inc_v3(domain,name)
#define __itt_counter_inc_v3_ptr       0
#define __itt_counter_inc_delta_v3(domain,name,delta)
#define __itt_counter_inc_delta_v3_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_counter_inc_v3_ptr       0
#define __itt_counter_inc_delta_v3_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup scope
 * @brief Describes the scope of an event object in the trace.
 */
typedef enum
{
    __itt_scope_unknown = 0,
    __itt_scope_global,
    __itt_scope_track_group,
    __itt_scope_track,
    __itt_scope_task,
    __itt_scope_marker,
} __itt_scope;

/** @cond exclude_from_documentation */
#define __itt_marker_scope_unknown  __itt_scope_unknown
#define __itt_marker_scope_global   __itt_scope_global
#define __itt_marker_scope_process  __itt_scope_track_group
#define __itt_marker_scope_thread   __itt_scope_track
#define __itt_marker_scope_task     __itt_scope_task
/** @endcond */

/**
 * @ingroup markers
 * @brief Create a marker instance.
 * @param[in] domain The domain for this marker
 * @param[in] id The instance ID for this marker, or __itt_null
 * @param[in] name The name for this marker
 * @param[in] scope The scope for this marker
 */
void ITTAPI __itt_marker(const __itt_domain *domain, __itt_id id, __itt_string_handle *name, __itt_scope scope);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, marker, (const __itt_domain *domain, __itt_id id, __itt_string_handle *name, __itt_scope scope))
#define __itt_marker(domain, id, name, scope) ITTNOTIFY_VOID_D3(marker,domain,id,name,scope)
#define __itt_marker_ptr					  ITTNOTIFY_NAME(marker)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_marker(domain,id,name,scope)
#define __itt_marker_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_marker_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup markers
 * @brief Create a marker instance.
 * @param[in] domain The domain for this marker
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] id The instance ID for this marker, or __itt_null
 * @param[in] name The name for this marker
 * @param[in] scope The scope for this marker
 */
void ITTAPI __itt_marker_ex(const __itt_domain *domain,  __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id, __itt_string_handle *name, __itt_scope scope);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, marker_ex,    (const __itt_domain *domain,  __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id id, __itt_string_handle *name, __itt_scope scope))
#define __itt_marker_ex(domain, clock_domain, timestamp, id, name, scope)    ITTNOTIFY_VOID_D5(marker_ex,domain,clock_domain,timestamp,id,name,scope)
#define __itt_marker_ex_ptr													 ITTNOTIFY_NAME(marker_ex)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_marker_ex(domain,clock_domain,timestamp,id,name,scope)
#define __itt_marker_ex_ptr    0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_marker_ex_ptr    0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup parameters
 * @brief Describes the type of metadata.
 */
typedef enum {
    __itt_metadata_unknown = 0,
    __itt_metadata_u64,     /**< Unsigned 64-bit integer */
    __itt_metadata_s64,     /**< Signed 64-bit integer */
    __itt_metadata_u32,     /**< Unsigned 32-bit integer */
    __itt_metadata_s32,     /**< Signed 32-bit integer */
    __itt_metadata_u16,     /**< Unsigned 16-bit integer */
    __itt_metadata_s16,     /**< Signed 16-bit integer */
    __itt_metadata_float,   /**< Signed 32-bit floating-point */
    __itt_metadata_double,  /**< SIgned 64-bit floating-point */
} __itt_metadata_type;

/**
 * @ingroup parameters
 * @brief Add metadata to an instance of a named entity.
 * @param[in] domain The domain controlling the call
 * @param[in] id The identifier of the instance to which the metadata is to be added, or __itt_null to add to the current task
 * @param[in] key The name of the metadata
 * @param[in] type The type of the metadata
 * @param[in] count The number of elements of the given type. If count == 0, no metadata will be added.
 * @param[in] data The metadata itself
*/
void ITTAPI __itt_metadata_add(const __itt_domain *domain, __itt_id id, __itt_string_handle *key, __itt_metadata_type type, size_t count, void *data);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, metadata_add, (const __itt_domain *domain, __itt_id id, __itt_string_handle *key, __itt_metadata_type type, size_t count, void *data))
#define __itt_metadata_add(domain,id,key,type,count,data)   ITTNOTIFY_VOID_D5(metadata_add,domain,id,key,type,count,data)
#define __itt_metadata_add_ptr            					ITTNOTIFY_NAME(metadata_add)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_metadata_add(domain,id,key,type,count,data)
#define __itt_metadata_add_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_metadata_add_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup parameters
 * @brief Add string metadata to an instance of a named entity.
 * @param[in] domain The domain controlling the call
 * @param[in] id The identifier of the instance to which the metadata is to be added, or __itt_null to add to the current task
 * @param[in] key The name of the metadata
 * @param[in] data The metadata itself
 * @param[in] length The number of characters in the string, or 0 if the length is unknown but the string is null-terminated 
*/
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __itt_metadata_str_addA(const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const char *data, size_t length);
void ITTAPI __itt_metadata_str_addW(const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const wchar_t *data, size_t length);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_metadata_str_add(domain, id, key, data, length)     __itt_metadata_str_addW(domain, id, key, data, length)
#  define __itt_metadata_str_add_ptr								__itt_metadata_str_addW_ptr
#else /* UNICODE */
#  define __itt_metadata_str_add(domain, id, key, data, length)     __itt_metadata_str_addA(domain, id, key, data, length)
#  define __itt_metadata_str_add_ptr __itt_metadata_str_addA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __itt_metadata_str_add(const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const char *data, size_t length);
#endif

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(ITTAPI, void, metadata_str_addA, (const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const char *data, size_t length))
ITT_STUBV(ITTAPI, void, metadata_str_addW, (const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const wchar_t *data, size_t length))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUBV(ITTAPI, void, metadata_str_add, (const __itt_domain *domain, __itt_id id, __itt_string_handle *key, const char *data, size_t length))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_addA(domain,id,key,data,length)  ITTNOTIFY_VOID_D4(metadata_str_addA,domain,id,key,data,length)
#define __itt_metadata_str_addA_ptr                         ITTNOTIFY_NAME(metadata_str_addA)
#define __itt_metadata_str_addW(domain,id,key,data,length)  ITTNOTIFY_VOID_D4(metadata_str_addW,domain,id,key,data,length)
#define __itt_metadata_str_addW_ptr                         ITTNOTIFY_NAME(metadata_str_addW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add(domain,id,key,data,length)   ITTNOTIFY_VOID_D4(metadata_str_add,domain,id,key,data,length)
#define __itt_metadata_str_add_ptr                          ITTNOTIFY_NAME(metadata_str_add)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_addA(domain,id,key,data,length) 
#define __itt_metadata_str_addA_ptr          0
#define __itt_metadata_str_addW(domain,id,key,data,length) 
#define __itt_metadata_str_addW_ptr          0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add(domain,id,key,data,length)
#define __itt_metadata_str_add_ptr 0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_addA_ptr 0
#define __itt_metadata_str_addW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup parameters
 * @brief Add metadata to an instance of a named entity.
 * @param[in] domain The domain controlling the call
 * @param[in] scope The scope of the instance to which the metadata is to be added

 * @param[in] id The identifier of the instance to which the metadata is to be added, or __itt_null to add to the current task
 
 * @param[in] key The name of the metadata
 * @param[in] type The type of the metadata
 * @param[in] count The number of elements of the given type. If count == 0, no metadata will be added.
 * @param[in] data The metadata itself
*/
void ITTAPI __itt_metadata_add_with_scope(const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, __itt_metadata_type type, size_t count, void *data);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, metadata_add_with_scope, (const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, __itt_metadata_type type, size_t count, void *data))
#define __itt_metadata_add_with_scope(domain,scope,key,type,count,data) ITTNOTIFY_VOID_D5(metadata_add_with_scope,domain,scope,key,type,count,data)
#define __itt_metadata_add_with_scope_ptr            					ITTNOTIFY_NAME(metadata_add_with_scope)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_metadata_add_with_scope(domain,scope,key,type,count,data)
#define __itt_metadata_add_with_scope_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_metadata_add_with_scope_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup parameters
 * @brief Add string metadata to an instance of a named entity.
 * @param[in] domain The domain controlling the call
 * @param[in] scope The scope of the instance to which the metadata is to be added

 * @param[in] id The identifier of the instance to which the metadata is to be added, or __itt_null to add to the current task

 * @param[in] key The name of the metadata
 * @param[in] data The metadata itself
 * @param[in] length The number of characters in the string, or -1 if the length is unknown but the string is null-terminated 
*/
#if ITT_PLATFORM==ITT_PLATFORM_WIN
void ITTAPI __itt_metadata_str_add_with_scopeA(const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const char *data, size_t length);
void ITTAPI __itt_metadata_str_add_with_scopeW(const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const wchar_t *data, size_t length);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_metadata_str_add_with_scope(domain, scope, key, data, length)     __itt_metadata_str_add_with_scopeW(domain, scope, key, data, length)
#  define __itt_metadata_str_add_with_scope_ptr									  __itt_metadata_str_add_with_scopeW_ptr
#else /* UNICODE */
#  define __itt_metadata_str_add_with_scope(domain, scope, key, data, length)     __itt_metadata_str_add_with_scopeA(domain, scope, key, data, length)
#  define __itt_metadata_str_add_with_scope_ptr									  __itt_metadata_str_add_with_scopeA_ptr
#endif /* UNICODE */
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
void ITTAPI __itt_metadata_str_add_with_scope(const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const char *data, size_t length);
#endif

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUBV(ITTAPI, void, metadata_str_add_with_scopeA, (const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const char *data, size_t length))
ITT_STUBV(ITTAPI, void, metadata_str_add_with_scopeW, (const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const wchar_t *data, size_t length))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUBV(ITTAPI, void, metadata_str_add_with_scope, (const __itt_domain *domain, __itt_scope scope, __itt_string_handle *key, const char *data, size_t length))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_add_with_scopeA(domain,scope,key,data,length) ITTNOTIFY_VOID_D4(metadata_str_add_with_scopeA,domain,scope,key,data,length)
#define __itt_metadata_str_add_with_scopeA_ptr                           ITTNOTIFY_NAME(metadata_str_add_with_scopeA)
#define __itt_metadata_str_add_with_scopeW(domain,scope,key,data,length) ITTNOTIFY_VOID_D4(metadata_str_add_with_scopeW,domain,scope,key,data,length)
#define __itt_metadata_str_add_with_scopeW_ptr                           ITTNOTIFY_NAME(metadata_str_add_with_scopeW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add_with_scope(domain,scope,key,data,length)  ITTNOTIFY_VOID_D4(metadata_str_add_with_scope,domain,scope,key,data,length)
#define __itt_metadata_str_add_with_scope_ptr                            ITTNOTIFY_NAME(metadata_str_add_with_scope)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_add_with_scopeA(domain,scope,key,data,length) 
#define __itt_metadata_str_add_with_scopeA_ptr  0
#define __itt_metadata_str_add_with_scopeW(domain,scope,key,data,length) 
#define __itt_metadata_str_add_with_scopeW_ptr  0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add_with_scope(domain,scope,key,data,length)
#define __itt_metadata_str_add_with_scope_ptr   0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_metadata_str_add_with_scopeA_ptr  0
#define __itt_metadata_str_add_with_scopeW_ptr  0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_metadata_str_add_with_scope_ptr   0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup relations
 * @brief The kind of relation between two instances is specified by the enumerated type __itt_relation.
 * Relations between instances can be added with an API call. The relation
 * API uses instance IDs. Relations can be added before or after the actual
 * instances are created and persist independently of the instances. This
 * is the motivation for having different lifetimes for instance IDs and
 * the actual instances.
 */
typedef enum
{
    __itt_relation_is_unknown = 0, 
    __itt_relation_is_dependent_on,         /**< "A is dependent on B" means that A cannot start until B completes */
    __itt_relation_is_sibling_of,           /**< "A is sibling of B" means that A and B were created as a group */
    __itt_relation_is_parent_of,            /**< "A is parent of B" means that A created B */
    __itt_relation_is_continuation_of,      /**< "A is continuation of B" means that A assumes the dependencies of B */
    __itt_relation_is_child_of,             /**< "A is child of B" means that A was created by B (inverse of is_parent_of) */
    __itt_relation_is_continued_by,         /**< "A is continued by B" means that B assumes the dependencies of A (inverse of is_continuation_of) */
    __itt_relation_is_predecessor_to        /**< "A is predecessor to B" means that B cannot start until A completes (inverse of is_dependent_on) */
} __itt_relation;

/**
 * @ingroup relations
 * @brief Add a relation to the current task instance.
 * The current task instance is the head of the relation.
 * @param[in] domain The domain controlling this call
 * @param[in] relation The kind of relation
 * @param[in] tail The ID for the tail of the relation
 */
void ITTAPI __itt_relation_add_to_current(const __itt_domain *domain, __itt_relation relation, __itt_id tail);

/**
 * @ingroup relations
 * @brief Add a relation between two instance identifiers.
 * @param[in] domain The domain controlling this call
 * @param[in] head The ID for the head of the relation
 * @param[in] relation The kind of relation
 * @param[in] tail The ID for the tail of the relation
 */
void ITTAPI __itt_relation_add(const __itt_domain *domain, __itt_id head, __itt_relation relation, __itt_id tail);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, relation_add_to_current, (const __itt_domain *domain, __itt_relation relation, __itt_id tail))
ITT_STUBV(ITTAPI, void, relation_add,            (const __itt_domain *domain, __itt_id head, __itt_relation relation, __itt_id tail))
#define __itt_relation_add_to_current(domain, relation, tail)	ITTNOTIFY_VOID_D2(relation_add_to_current,domain,relation,tail)
#define __itt_relation_add_to_current_ptr						ITTNOTIFY_NAME(relation_add_to_current)
#define __itt_relation_add(domain, head, relation, tail)        ITTNOTIFY_VOID_D3(relation_add,domain,head,relation,tail)
#define __itt_relation_add_ptr									ITTNOTIFY_NAME(relation_add)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_relation_add_to_current(domain,relation,tail)
#define __itt_relation_add_to_current_ptr 0
#define __itt_relation_add(domain,head,relation,tail)
#define __itt_relation_add_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_relation_add_to_current_ptr 0
#define __itt_relation_add_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @ingroup clockdomain
 * @brief Add a relation to the current task instance.
 * The current task instance is the head of the relation.
 * @param[in] domain The domain controlling this call
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] relation The kind of relation
 * @param[in] tail The ID for the tail of the relation
 */
void ITTAPI __itt_relation_add_to_current_ex(const __itt_domain *domain,  __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_relation relation, __itt_id tail);

/**
 * @ingroup clockdomain
 * @brief Add a relation between two instance identifiers.
 * @param[in] domain The domain controlling this call
 * @param[in] clock_domain The clock domain controlling the execution of this call.
 * @param[in] timestamp The user defined timestamp.
 * @param[in] head The ID for the head of the relation
 * @param[in] relation The kind of relation
 * @param[in] tail The ID for the tail of the relation
 */
void ITTAPI __itt_relation_add_ex(const __itt_domain *domain,  __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id head, __itt_relation relation, __itt_id tail);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, relation_add_to_current_ex, (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_relation relation, __itt_id tail))
ITT_STUBV(ITTAPI, void, relation_add_ex,            (const __itt_domain *domain, __itt_clock_domain* clock_domain, unsigned long long timestamp, __itt_id head, __itt_relation relation, __itt_id tail))
#define __itt_relation_add_to_current_ex(domain, clock_domain, timestamp, relation, tail)		ITTNOTIFY_VOID_D4(relation_add_to_current_ex,domain,clock_domain,timestamp,relation,tail)
#define __itt_relation_add_to_current_ex_ptr													ITTNOTIFY_NAME(relation_add_to_current_ex)
#define __itt_relation_add_ex(domain, clock_domain, timestamp, head, relation, tail)			ITTNOTIFY_VOID_D5(relation_add_ex,domain,clock_domain,timestamp,head,relation,tail)
#define __itt_relation_add_ex_ptr																ITTNOTIFY_NAME(relation_add_ex)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_relation_add_to_current_ex(domain,clock_domain,timestame,relation,tail)
#define __itt_relation_add_to_current_ex_ptr 0
#define __itt_relation_add_ex(domain,clock_domain,timestamp,head,relation,tail)
#define __itt_relation_add_ex_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_relation_add_to_current_ex_ptr 0
#define __itt_relation_add_ex_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/** @cond exclude_from_documentation */
typedef enum
{
    __itt_track_group_type_normal = 0
} __itt_track_group_type;
/** @endcond */


/** @cond exclude_from_documentation */
typedef struct ___itt_track_group
{
    __itt_string_handle* name; /*!< Name of the track group */
    struct ___itt_track* track; /*!< List of child tracks  */
    __itt_track_group_type tgtype; /*!< Type of the track group */
    int   extra1; /*!< Reserved. Mast be zero   */
    void* extra2; /*!< Reserved. Mast be zero   */
    struct ___itt_track_group* next;
} __itt_track_group;
/** @endcond */

/**
 * @brief Placeholder for custom track types. Currently, "normal" custom track
 * is the only available track type.
 */
typedef enum  
{
    __itt_track_type_normal = 0
#ifdef INTEL_ITTNOTIFY_API_PRIVATE
    , __itt_track_type_queue
#endif
} __itt_track_type;

/** @cond exclude_from_documentation */
typedef struct ___itt_track
{
    __itt_string_handle* name; /*!< Name of the track group */
    __itt_track_group* group; /*!< Parent group to a track */
    __itt_track_type ttype; /*!< Type of the track */
    int   extra1; /*!< Reserved. Mast be zero   */
    void* extra2; /*!< Reserved. Mast be zero   */
    struct ___itt_track* next;
} __itt_track;
/** @endcond */

/**
 * @brief Create logical track group.
 */
__itt_track_group* ITTAPI __itt_track_group_create(__itt_string_handle* name, __itt_track_group_type track_group_type);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUB(ITTAPI, __itt_track_group*, track_group_create, (__itt_string_handle* name, __itt_track_group_type track_group_type))
#define __itt_track_group_create				 ITTNOTIFY_DATA(track_group_create)
#define __itt_track_group_create_ptr			 ITTNOTIFY_NAME(track_group_create)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_track_group_create(name)  (__itt_track_group*)0
#define __itt_track_group_create_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_track_group_create_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Create logical track.
 */
__itt_track* ITTAPI __itt_track_create(__itt_track_group* track_group, __itt_string_handle* name, __itt_track_type track_type);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUB(ITTAPI, __itt_track*, track_create, (__itt_track_group* track_group,__itt_string_handle* name, __itt_track_type track_type))
#define __itt_track_create					     ITTNOTIFY_DATA(track_create)
#define __itt_track_create_ptr					 ITTNOTIFY_NAME(track_create)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_track_create(track_group,name,track_type)  (__itt_track*)0
#define __itt_track_create_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_track_create_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Set the logical track.
 */
void ITTAPI __itt_set_track(__itt_track* track);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUBV(ITTAPI, void, set_track, (__itt_track *track))
#define __itt_set_track     ITTNOTIFY_VOID(set_track)
#define __itt_set_track_ptr ITTNOTIFY_NAME(set_track)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_set_track(track)
#define __itt_set_track_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_set_track_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/* ========================================================================== */
/** @cond exclude_from_gpa_documentation */
/**
 * @defgroup events Events
 * @ingroup public
 * Events group
 * @{
 */
/** @brief user event type */
typedef int __itt_event;

/**
 * @brief Create an event notification.
 * @note name or namelen being null/name and namelen not matching, user event feature not enabled
 * @return non-zero event identifier upon success and __itt_err otherwise
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
__itt_event LIBITTAPI __itt_event_createA(const char    *name, int namelen);
__itt_event LIBITTAPI __itt_event_createW(const wchar_t *name, int namelen);
#if defined(UNICODE) || defined(_UNICODE)
#  define __itt_event_create     __itt_event_createW
#  define __itt_event_create_ptr __itt_event_createW_ptr
#else
#  define __itt_event_create     __itt_event_createA
#  define __itt_event_create_ptr __itt_event_createA_ptr
#endif /* UNICODE */
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
__itt_event LIBITTAPI __itt_event_create(const char *name, int namelen);
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
#if ITT_PLATFORM==ITT_PLATFORM_WIN
ITT_STUB(LIBITTAPI, __itt_event, event_createA, (const char    *name, int namelen))
ITT_STUB(LIBITTAPI, __itt_event, event_createW, (const wchar_t *name, int namelen))
#else  /* ITT_PLATFORM==ITT_PLATFORM_WIN */
ITT_STUB(LIBITTAPI, __itt_event, event_create,  (const char    *name, int namelen))
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_event_createA     ITTNOTIFY_DATA(event_createA)
#define __itt_event_createA_ptr ITTNOTIFY_NAME(event_createA)
#define __itt_event_createW     ITTNOTIFY_DATA(event_createW)
#define __itt_event_createW_ptr ITTNOTIFY_NAME(event_createW)
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_event_create      ITTNOTIFY_DATA(event_create)
#define __itt_event_create_ptr  ITTNOTIFY_NAME(event_create)
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#else  /* INTEL_NO_ITTNOTIFY_API */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_event_createA(name, namelen) (__itt_event)0
#define __itt_event_createA_ptr 0
#define __itt_event_createW(name, namelen) (__itt_event)0
#define __itt_event_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_event_create(name, namelen)  (__itt_event)0
#define __itt_event_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define __itt_event_createA_ptr 0
#define __itt_event_createW_ptr 0
#else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#define __itt_event_create_ptr  0
#endif /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an event occurrence.
 * @return __itt_err upon failure (invalid event id/user event feature not enabled)
 */
int LIBITTAPI __itt_event_start(__itt_event event);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUB(LIBITTAPI, int, event_start, (__itt_event event))
#define __itt_event_start     ITTNOTIFY_DATA(event_start)
#define __itt_event_start_ptr ITTNOTIFY_NAME(event_start)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_event_start(event) (int)0
#define __itt_event_start_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_event_start_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */

/**
 * @brief Record an event end occurrence.
 * @note It is optional if events do not have durations.
 * @return __itt_err upon failure (invalid event id/user event feature not enabled)
 */
int LIBITTAPI __itt_event_end(__itt_event event);

/** @cond exclude_from_documentation */
#ifndef INTEL_NO_MACRO_BODY
#ifndef INTEL_NO_ITTNOTIFY_API
ITT_STUB(LIBITTAPI, int, event_end, (__itt_event event))
#define __itt_event_end     ITTNOTIFY_DATA(event_end)
#define __itt_event_end_ptr ITTNOTIFY_NAME(event_end)
#else  /* INTEL_NO_ITTNOTIFY_API */
#define __itt_event_end(event) (int)0
#define __itt_event_end_ptr 0
#endif /* INTEL_NO_ITTNOTIFY_API */
#else  /* INTEL_NO_MACRO_BODY */
#define __itt_event_end_ptr 0
#endif /* INTEL_NO_MACRO_BODY */
/** @endcond */
/** @} events group */
/** @endcond */
/** @cond exclude_from_documentation */
#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @endcond */

#endif /* _ITTNOTIFY_H_ */
