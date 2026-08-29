#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>

/// Low-level memory allocator that manages whole memory pages from the operating system.
///
/// This allocator requests memory directly from the OS in page-aligned chunks, typically 4KB or larger.
/// It's designed for scenarios where you need large, aligned memory blocks with minimal overhead.
/// Unlike general-purpose allocators, this operates at the virtual memory level and may reserve
/// address space that isn't immediately committed to physical memory.
///
/// Use this allocator when:
/// - You need very large memory blocks (typically >64KB)
/// - Memory must be page-aligned for specific algorithms or OS APIs
/// - You want to minimize memory overhead for large allocations
/// - You're implementing higher-level allocators that need backing storage
class EZ_FOUNDATION_DLL ezPageAllocator
{
public:
  /// Allocates a page-aligned memory block of at least the specified size.
  static void* AllocatePage(size_t uiSize);

  /// Deallocates a memory block previously allocated with AllocatePage.
  static void DeallocatePage(void* pPtr);

  /// Returns the allocator identifier for tracking and debugging purposes.
  static ezAllocatorId GetId();
};
