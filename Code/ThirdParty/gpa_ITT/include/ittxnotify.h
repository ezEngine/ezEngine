/*
    Copyright(c) 2011 Intel Corporation.  All Rights Reserved.

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
#ifndef _ITTXNOTIFY_H_
#define _ITTXNOTIFY_H_

#include "ittnotify.h"

#ifdef __cplusplus

class __ittx_scoped_task
{
    const __itt_domain* mDomain;
public:
    INLINE __ittx_scoped_task(const __itt_domain *domain, __itt_id taskid, __itt_id parentid, __itt_string_handle *name):
    mDomain(domain)
    {
        __itt_task_begin(domain, taskid, parentid, name);
    }
    INLINE ~__ittx_scoped_task()
    {
        __itt_task_end(mDomain);
    }
private:
    __ittx_scoped_task();
    __ittx_scoped_task(const __ittx_scoped_task&);
    __ittx_scoped_task& operator = (const __ittx_scoped_task&);
};

class __ittx_scoped_task_fn
{
    const __itt_domain* mDomain;
public:
    INLINE __ittx_scoped_task_fn(const __itt_domain *domain, __itt_id taskid, __itt_id parentid, void* fn):
    mDomain(domain)
    {
        __itt_task_begin_fn(domain, taskid, parentid, fn);
    }
    INLINE ~__ittx_scoped_task_fn()
    {
        __itt_task_end(mDomain);
    }
private:
    __ittx_scoped_task_fn();
    __ittx_scoped_task_fn(const __ittx_scoped_task_fn&);
    __ittx_scoped_task_fn& operator = (const __ittx_scoped_task_fn&);
};

#ifndef INTEL_NO_ITTNOTIFY_API
#define ITTX_SCOPED_TASK(domain, taskid, parentid, name) \
    static __itt_string_handle* _ittx_scoped_task_string_handle_##name = __itt_string_handle_create(name); \
    __ittx_scoped_task _ittx_scoped_task_##name(domain, taskid, parentid, _ittx_scoped_task_string_handle_##name)

#define ITTX_SCOPED_TASK_FN(domain, taskid, parentid, fn) \
    __ittx_scoped_task_fn _ittx_scoped_task_fn(domain, taskid, parentid, fn)

#define ITTX_SCOPED_TASK_NAMED(domain, name) \
    static __itt_string_handle* _ittx_scoped_task_string_handle_##name = __itt_string_handle_create(name); \
    __ittx_scoped_task _ittx_scoped_task_##name(domain, __itt_null, __itt_null, _ittx_scoped_task_string_handle_##name)

#define ITTX_SCOPED_TASK_NAMED_WITH_ID(domain, taskid, name) \
    static __itt_string_handle* _ittx_scoped_task_string_handle_##name = __itt_string_handle_create(name); \
    __ittx_scoped_task _ittx_scoped_task_##name(domain, taskid, __itt_null, _ittx_scoped_task_string_handle_##name)
#else
#define ITTX_SCOPED_TASK(domain, taskid, parentid, name)
#define ITTX_SCOPED_TASK_FN(domain, taskid, parentid, fn)
#define ITTX_SCOPED_TASK_NAMED(domain, name)
#define ITTX_SCOPED_TASK_NAMED_WITH_ID(domain, taskid, name)
#endif /*INTEL_NO_ITTNOTIFY_API*/

#ifdef INTEL_ITTNOTIFY_API_PRIVATE

#include "ittxutils.h"
#if ITT_PLATFORM != ITT_PLATFORM_WIN
#define strncpy_s(dest,len,src,cnt) strncpy(dest,src,cnt)
#define sprintf_s(buf,len,fmt,...) sprintf(buf,fmt,##__VA_ARGS__)
#endif /*ITT_PLATFORM*/ 


class __ittx_task_state
{
public:
    friend __ittx_task_state* __ittx_task_state_create(const __itt_domain* domain, const char* state_name);

    ~__ittx_task_state()
    {
        delete m_Name;
    }

    const char* GetName() const { return m_Name; }
    unsigned GetStateCode() const { return m_StateCode; }

private:    
    __ittx_task_state(const char* key) : m_Name(NULL)
    {
        static volatile int nextStateCode = 0;
        m_StateCode = __ittx_atomic_increment(&nextStateCode);

        size_t length = strlen(key) + 1;
        try
        {
            m_Name = new char[length];
            strcpy_s(m_Name, length, key);
        }
        catch (...) // new might throw a bad_alloc
        {
        	delete m_Name;
            throw;
        }
    }

    __ittx_task_state();
    __ittx_task_state(const __ittx_task_state& state);
    __ittx_task_state& operator=(const __ittx_task_state& state);

    char* m_Name;
    int m_StateCode;
};

#ifndef INTEL_NO_ITTNOTIFY_API

static const char* const task_state_name_str = "<metadata uri=\"com.intel.task_state_name\" index=\"%d\"/>";
static const char* const task_state_str =      "<metadata uri=\"com.intel.gpa.task_state\"/>";
static const char* const task_state_ex_str =   "<metadata uri=\"com.intel.gpa.task_state_ex\"/>";

static const __itt_string_handle* const task_state_str_handle = __itt_string_handle_createA(task_state_str);
static const __itt_string_handle* const task_state_ex_str_handle = __itt_string_handle_createA(task_state_ex_str);

// Defines a task state by name.
INLINE __ittx_task_state* __ittx_task_state_create(const __itt_domain* domain, const char* state_name)
{
    if (state_name == NULL || strlen(state_name) == 0)
    {
        return NULL;
    }

    __ittx_task_state* newState = new __ittx_task_state(state_name);

    char buffer[256];
    sprintf_s(buffer, 256, task_state_name_str, newState->GetStateCode());
    __itt_string_handle* create_handle = __itt_string_handle_createA(buffer);
    size_t len = strlen(newState->GetName());
    __itt_metadata_str_add_with_scopeA(domain, __itt_scope_global, create_handle, newState->GetName(), len);

    return newState;
}

// For the current task in scope (or the task indicated by taskid), set the current task state.
INLINE void __ittx_task_set_state(const __itt_domain* domain, __itt_id taskid, __ittx_task_state* task_state)
{
    if (task_state == NULL)
    {
        return;
    }

    unsigned long long state_data[2];
    state_data[0] = __ittx_get_current_time();
    state_data[1] = task_state->GetStateCode();   
    __itt_metadata_add(domain, taskid, const_cast<__itt_string_handle*>(task_state_str_handle), __itt_metadata_u64, 2, state_data);
}

// Set state for custom clock domains.
INLINE void __ittx_task_set_state(const __itt_domain* domain, __itt_clock_domain* clockdomain, unsigned long long timestamp,
                      __itt_id taskid, __ittx_task_state* task_state)
{
    if (task_state == NULL)
    {
        return;
    }

    unsigned long long state_data[3];
    state_data[0] = (unsigned long long) timestamp;
    state_data[1] = (unsigned long long) clockdomain;
    state_data[2] = task_state->GetStateCode();   
    __itt_metadata_add(domain, taskid, const_cast<__itt_string_handle*>(task_state_ex_str_handle), __itt_metadata_u64, 3, state_data);
}

// Set the default task state for the current track.
INLINE void __ittx_set_default_state(const __itt_domain* domain, __ittx_task_state* task_state)
{
     if (task_state == NULL)
     {
         return;
     }

     unsigned long long state_data[2];
     state_data[0] = __ittx_get_current_time();
     state_data[1] = task_state->GetStateCode();
     __itt_metadata_add_with_scope(domain, __itt_scope_track, const_cast<__itt_string_handle*>(task_state_str_handle), __itt_metadata_u64, 2, state_data);
}

#else

#define __ittx_task_state_create(domain,state_name) 0
#define __ittx_task_set_state
#define __ittx_task_set_state
#define __ittx_set_default_state

#endif /*INTEL_NO_ITTNOTIFY_API*/
#endif /*INTEL_ITTNOTIFY_API_PRIVATE*/ 

#endif /*__cplusplus*/
#endif /*_ITTXNOTIFY_H_*/