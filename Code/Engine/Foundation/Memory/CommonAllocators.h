
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AlignedHeapAllocation.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Memory/Policies/ProxyAllocation.h>


/// \brief Default heap allocator
typedef ezAllocator<ezMemoryPolicies::ezAlignedHeapAllocation> ezAlignedHeapAllocator;

/// \brief Default heap allocator
typedef ezAllocator<ezMemoryPolicies::ezHeapAllocation> ezHeapAllocator;

/// \brief Proxy allocator
typedef ezAllocator<ezMemoryPolicies::ezProxyAllocation> ezProxyAllocator;

