
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/GuardedAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>


/// \brief Default heap allocator
using ezAlignedHeapAllocator = ezAllocator<ezMemoryPolicies::ezAlignedHeapAllocation>;

/// \brief Default heap allocator
using ezHeapAllocator = ezAllocator<ezMemoryPolicies::ezHeapAllocation>;

/// \brief Guarded allocator
using ezGuardedAllocator = ezAllocator<ezMemoryPolicies::ezGuardedAllocation>;

/// \brief Proxy allocator
using ezProxyAllocator = ezAllocator<ezMemoryPolicies::ezProxyAllocation>;
