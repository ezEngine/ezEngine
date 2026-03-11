#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Memory/LinearAllocator.h>
#include <Foundation/Memory/Policies/AllocPolicyStack.h>

struct alignas(EZ_ALIGNMENT_MINIMUM) NonAlignedVector
{
  EZ_DECLARE_POD_TYPE();

  NonAlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
};

struct alignas(16) AlignedVector
{
  EZ_DECLARE_POD_TYPE();

  AlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
  float w;
};

template <typename T>
void TestAlignmentHelper(size_t uiExpectedAlignment)
{
  ezAllocator* pAllocator = ezFoundation::GetAlignedAllocator();
  EZ_TEST_BOOL(pAllocator != nullptr);

  size_t uiAlignment = alignof(T);
  EZ_TEST_INT(uiAlignment, uiExpectedAlignment);

  T testOnStack = T();
  EZ_TEST_BOOL(ezMemoryUtils::IsAligned(&testOnStack, uiExpectedAlignment));

  T* pTestBuffer = EZ_NEW_RAW_BUFFER(pAllocator, T, 32);
  ezArrayPtr<T> TestArray = EZ_NEW_ARRAY(pAllocator, T, 32);

  // default constructor should be called even if we declare as a pod type
  EZ_TEST_FLOAT(TestArray[0].x, 5.0f, 0.0f);
  EZ_TEST_FLOAT(TestArray[0].y, 6.0f, 0.0f);
  EZ_TEST_FLOAT(TestArray[0].z, 8.0f, 0.0f);

  EZ_TEST_BOOL(ezMemoryUtils::IsAligned(pTestBuffer, uiExpectedAlignment));
  EZ_TEST_BOOL(ezMemoryUtils::IsAligned(TestArray.GetPtr(), uiExpectedAlignment));

  size_t uiExpectedSize = sizeof(T) * 32;

  if constexpr (ezAllocatorTrackingMode::Default >= ezAllocatorTrackingMode::AllocationStats)
  {
    EZ_TEST_INT(pAllocator->AllocatedSize(pTestBuffer), uiExpectedSize);

    ezAllocator::Stats stats = pAllocator->GetStats();
    EZ_TEST_INT(stats.m_uiAllocationSize, uiExpectedSize * 2);
    EZ_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 2);
  }

  EZ_DELETE_ARRAY(pAllocator, TestArray);
  EZ_DELETE_RAW_BUFFER(pAllocator, pTestBuffer);

  if constexpr (ezAllocatorTrackingMode::Default >= ezAllocatorTrackingMode::Basics)
  {
    ezAllocator::Stats stats = pAllocator->GetStats();
    EZ_TEST_INT(stats.m_uiAllocationSize, 0);
    EZ_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 0);
  }
}

EZ_CREATE_SIMPLE_TEST_GROUP(Memory);

EZ_CREATE_SIMPLE_TEST(Memory, Allocator)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Alignment")
  {
    TestAlignmentHelper<NonAlignedVector>(EZ_ALIGNMENT_MINIMUM);
    TestAlignmentHelper<AlignedVector>(16);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LargeBlockAllocator")
  {
    enum
    {
      BLOCK_SIZE_IN_BYTES = 4096 * 4
    };
    const ezUInt32 uiPageSize = ezSystemInformation::Get().GetMemoryPageSize();

    ezLargeBlockAllocator<BLOCK_SIZE_IN_BYTES> allocator("Test", ezFoundation::GetDefaultAllocator(), ezAllocatorTrackingMode::AllocationStats);

    ezDynamicArray<ezDataBlock<int, BLOCK_SIZE_IN_BYTES>> blocks;
    blocks.Reserve(1000);

    for (ezUInt32 i = 0; i < 17; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(block.m_pData, uiPageSize)); // test page alignment
      EZ_TEST_INT(block.m_uiCount, 0);

      blocks.PushBack(block);
    }

    ezAllocator::Stats stats = allocator.GetStats();

    EZ_TEST_BOOL(stats.m_uiNumAllocations == 17);
    EZ_TEST_BOOL(stats.m_uiNumDeallocations == 0);
    EZ_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (ezUInt32 i = 0; i < 200; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      blocks.PushBack(block);
    }

    for (ezUInt32 i = 0; i < 200; ++i)
    {
      allocator.DeallocateBlock(blocks.PeekBack());
      blocks.PopBack();
    }

    stats = allocator.GetStats();

    EZ_TEST_BOOL(stats.m_uiNumAllocations == 217);
    EZ_TEST_BOOL(stats.m_uiNumDeallocations == 200);
    EZ_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (ezUInt32 i = 0; i < 2000; ++i)
    {
      ezUInt32 uiAction = rand() % 2;
      if (uiAction == 0)
      {
        blocks.PushBack(allocator.AllocateBlock<int>());
      }
      else if (blocks.GetCount() > 0)
      {
        ezUInt32 uiIndex = rand() % blocks.GetCount();
        auto block = blocks[uiIndex];

        allocator.DeallocateBlock(block);

        blocks.RemoveAtAndSwap(uiIndex);
      }
    }

    for (ezUInt32 i = 0; i < blocks.GetCount(); ++i)
    {
      allocator.DeallocateBlock(blocks[i]);
    }

    stats = allocator.GetStats();

    EZ_TEST_BOOL(stats.m_uiNumAllocations - stats.m_uiNumDeallocations == 0);
    EZ_TEST_BOOL(stats.m_uiAllocationSize == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LinearAllocator")
  {
    ezLinearAllocator<> allocator("TestLinearAllocator", ezFoundation::GetAlignedAllocator(), 4096);

    void* blocks[8];
    for (size_t i = 0; i < EZ_ARRAY_SIZE(blocks); i++)
    {
      size_t size = i + 1;
      blocks[i] = allocator.Allocate(size, sizeof(void*), nullptr);
      EZ_TEST_BOOL(blocks[i] != nullptr);
      if (i > 0)
      {
        EZ_TEST_BOOL((ezUInt8*)blocks[i - 1] + (size - 1) <= blocks[i]);
      }
    }

    for (size_t i = EZ_ARRAY_SIZE(blocks); i--;)
    {
      allocator.Deallocate(blocks[i]);
    }

    size_t sizes[] = {128, 128, 4096, 1024, 1024, 16000, 512, 512, 768, 768, 16000, 16000, 16000, 16000};
    void* allocs[EZ_ARRAY_SIZE(sizes)];
    for (size_t i = 0; i < EZ_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      EZ_TEST_BOOL(allocs[i] != nullptr);
    }
    for (size_t i = EZ_ARRAY_SIZE(sizes); i--;)
    {
      allocator.Deallocate(allocs[i]);
    }
    allocator.Reset();

    for (size_t i = 0; i < EZ_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      EZ_TEST_BOOL(allocs[i] != nullptr);
    }
    allocator.Reset();
    allocs[0] = allocator.Allocate(8, sizeof(void*), nullptr);
    EZ_TEST_BOOL(allocs[0] < allocs[1]);
    allocator.Deallocate(allocs[0]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LinearAllocator with non-PODs")
  {
    ezLinearAllocator<> allocator("TestLinearAllocator", ezFoundation::GetAlignedAllocator(), 4096);

    ezDynamicArray<ezConstructionCounter*> counters;
    counters.Reserve(100);

    for (ezUInt32 i = 0; i < 100; ++i)
    {
      counters.PushBack(EZ_NEW(&allocator, ezConstructionCounter));
    }

    for (ezUInt32 i = 0; i < 100; ++i)
    {
      EZ_NEW(&allocator, NonAlignedVector);
    }

    EZ_TEST_BOOL(ezConstructionCounter::HasConstructed(100));

    for (ezUInt32 i = 0; i < 50; ++i)
    {
      EZ_DELETE(&allocator, counters[i * 2]);
    }

    EZ_TEST_BOOL(ezConstructionCounter::HasDestructed(50));

    allocator.Reset();

    EZ_TEST_BOOL(ezConstructionCounter::HasDestructed(50));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TempAllocator")
  {
    using TempAllocatorType = ezAllocatorWithPolicy<ezAllocPolicyStack<true>>;

    // Basic LIFO allocate/deallocate
    {
      TempAllocatorType allocator("TestTempAllocator", ezFoundation::GetAlignedAllocator());

      ezUInt32* pA = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 64);
      ezUInt32* pB = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 128);
      ezUInt32* pC = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 256);

      EZ_TEST_BOOL(pA != nullptr);
      EZ_TEST_BOOL(pB != nullptr);
      EZ_TEST_BOOL(pC != nullptr);

      // All within the same bucket, so addresses should be ascending
      EZ_TEST_BOOL(pA < pB);
      EZ_TEST_BOOL(pB < pC);

      // make copy of pA to check if it gets reused after deallocation
      ezUInt32* pExpectedAlloc = pA;

      // Deallocate in LIFO order
      EZ_DELETE_RAW_BUFFER(&allocator, pC);
      EZ_DELETE_RAW_BUFFER(&allocator, pB);
      EZ_DELETE_RAW_BUFFER(&allocator, pA);

      ezUInt32* pD = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 64);
      EZ_TEST_BOOL(pD != nullptr);
      EZ_TEST_BOOL(pD == pExpectedAlloc); // should reuse the same memory

      EZ_DELETE_RAW_BUFFER(&allocator, pD);
    }

    // Out-of-order deallocation
    {
      TempAllocatorType allocator("TestTempAllocator", ezFoundation::GetAlignedAllocator());

      ezUInt32* ptrs[5];
      for (int i = 0; i < 5; ++i)
      {
        ptrs[i] = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 32);
        EZ_TEST_BOOL(ptrs[i] != nullptr);
      }

      ezUInt32* pExpectedAlloc = ptrs[1]; // should be reused after free

      // Free indices 1, 2, 3 out of order (all become nullptr entries)
      EZ_DELETE_RAW_BUFFER(&allocator, ptrs[1]);
      EZ_DELETE_RAW_BUFFER(&allocator, ptrs[2]);
      EZ_DELETE_RAW_BUFFER(&allocator, ptrs[3]);

      // Free index 4 (last) - should cascade and also pop the nullptr entries for 3, 2, 1
      EZ_DELETE_RAW_BUFFER(&allocator, ptrs[4]);

      ezUInt32* pD = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 64);
      EZ_TEST_BOOL(pD != nullptr);
      EZ_TEST_BOOL(pD == pExpectedAlloc); // should reuse the same memory

      EZ_DELETE_RAW_BUFFER(&allocator, ptrs[0]);
      EZ_DELETE_RAW_BUFFER(&allocator, pD);
    }

    // Multiple buckets (allocations that span across bucket boundaries)
    {
      TempAllocatorType allocator("TestTempAllocator", ezFoundation::GetAlignedAllocator());

      // Fill first bucket (1024*1024 bytes max)
      ezUInt32* pA = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 128 * 1024);
      ezUInt32* pB = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 128 * 1024);
      EZ_TEST_BOOL(pA != nullptr);
      EZ_TEST_BOOL(pB != nullptr);

      // This should trigger a new bucket
      ezUInt32* pC = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 64);
      EZ_TEST_BOOL(pC != nullptr);

      // Deallocate in LIFO order - should roll back across bucket boundary
      EZ_DELETE_RAW_BUFFER(&allocator, pC);
      EZ_DELETE_RAW_BUFFER(&allocator, pB);
      EZ_DELETE_RAW_BUFFER(&allocator, pA);
    }

    // Large allocations that exceed max allocation size (parent allocator fallback)
    {
      TempAllocatorType allocator("TestTempAllocator", ezFoundation::GetAlignedAllocator());

      // This allocation exceeds the max allocation size, so it goes to the parent allocator
      ezUInt32* pLarge = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 1024 * 1024);
      EZ_TEST_BOOL(pLarge != nullptr);

      // Mix a normal allocation in between
      ezUInt32* pSmall = EZ_NEW_RAW_BUFFER(&allocator, ezUInt32, 64);
      EZ_TEST_BOOL(pSmall != nullptr);

      // Deallocating the large one should go to parent (not found in m_Allocations search, falls through)
      EZ_DELETE_RAW_BUFFER(&allocator, pLarge);

      EZ_DELETE_RAW_BUFFER(&allocator, pSmall);
    }
  }
}
