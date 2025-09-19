
#pragma once

/// \file

#include <Foundation/Memory/AllocatorWithPolicy.h>

#include <Foundation/Memory/Policies/AllocPolicyAlignedHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Memory/Policies/AllocPolicyProxy.h>


/// \brief Default heap allocator with alignment support.
///
/// This allocator supports arbitrary alignment requirements.
/// Uses the system heap with platform-specific alignment functions.
/// This is mainly needed when allocating GPU resources or SIMD data structures,
/// which require 16 byte alignment.
using ezAlignedHeapAllocator = ezAllocatorWithPolicy<ezAllocPolicyAlignedHeap>;

/// \brief Basic heap allocator without special alignment support.
///
/// Faster than ezAlignedHeapAllocator but only supports natural alignment (alignof(T)).
/// The is the recommended allocator for general purpose use.
using ezHeapAllocator = ezAllocatorWithPolicy<ezAllocPolicyHeap>;

/// \brief Debug allocator that adds guard pages around allocations.
///
/// Detects buffer overruns and use-after-free bugs by placing guard pages before and after
/// each allocation. Significantly slower and uses much more memory, only for debugging.
/// Will trigger access violations on memory corruption.
using ezGuardingAllocator = ezAllocatorWithPolicy<ezAllocPolicyGuarding>;

/// \brief Proxy allocator that forwards all operations to another allocator.
///
/// Useful for implementing statistics collection without modifying the underlying allocator.
using ezProxyAllocator = ezAllocatorWithPolicy<ezAllocPolicyProxy>;
