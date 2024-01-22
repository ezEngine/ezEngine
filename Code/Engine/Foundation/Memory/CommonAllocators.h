
#pragma once

/// \file

#include <Foundation/Memory/AllocatorWithPolicy.h>

#include <Foundation/Memory/Policies/AllocPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyProxy.h>


/// \brief Default heap allocator
using ezAlignedHeapAllocator = ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyAlignedHeap>;

/// \brief Default heap allocator
using ezHeapAllocator = ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyHeap>;

/// \brief Guarded allocator
using ezGuardedAllocator = ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyGuarding>;

/// \brief Proxy allocator
using ezProxyAllocator = ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyProxy>;
