
#pragma once

/// \file

#include <Foundation/Memory/AllocatorWithPolicy.h>

#include <Foundation/Memory/Policies/AllocPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyProxy.h>


/// \brief Default heap allocator
using ezAlignedHeapAllocator = ezAllocatorWithPolicy<ezAllocPolicyAlignedHeap>;

/// \brief Default heap allocator
using ezHeapAllocator = ezAllocatorWithPolicy<ezAllocPolicyHeap>;

/// \brief Guarded allocator
using ezGuardingAllocator = ezAllocatorWithPolicy<ezAllocPolicyGuarding>;

/// \brief Proxy allocator
using ezProxyAllocator = ezAllocatorWithPolicy<ezAllocPolicyProxy>;
