
#pragma once

/// \file

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Memory/Policies/AllocPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyProxy.h>


/// \brief Default heap allocator
using ezAlignedHeapAllocator = ezAllocator<ezMemoryPolicies::ezAllocPolicyAlignedHeap>;

/// \brief Default heap allocator
using ezHeapAllocator = ezAllocator<ezMemoryPolicies::ezAllocPolicyHeap>;

/// \brief Guarded allocator
using ezGuardedAllocator = ezAllocator<ezMemoryPolicies::ezAllocPolicyGuarding>;

/// \brief Proxy allocator
using ezProxyAllocator = ezAllocator<ezMemoryPolicies::ezAllocPolicyProxy>;
