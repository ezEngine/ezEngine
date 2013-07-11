
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>

#include <Foundation/Memory/Policies/Tracking.h>

#include <Foundation/Threading/Mutex.h>

/// \brief Default heap allocator without guard bounds checking and with simple tracking
typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryPolicies::ezSimpleTracking, ezNoMutex> ezHeapAllocator;

/// \brief Proxy allocator with simple tracking
typedef ezAllocator<ezMemoryPolicies::ezProxyAllocation, ezMemoryPolicies::ezSimpleTracking, ezNoMutex> ezProxyAllocator;
