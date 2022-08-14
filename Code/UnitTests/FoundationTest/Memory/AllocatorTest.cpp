#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Memory/StackAllocator.h>

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
  ezAllocatorBase* pAllocator = ezFoundation::GetAlignedAllocator();
  EZ_TEST_BOOL(pAllocator != nullptr);

  size_t uiAlignment = EZ_ALIGNMENT_OF(T);
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

#if EZ_ENABLED(EZ_USE_ALLOCATION_TRACKING)
  EZ_TEST_INT(pAllocator->AllocatedSize(pTestBuffer), uiExpectedSize);

  ezAllocatorBase::Stats stats = pAllocator->GetStats();
  EZ_TEST_INT(stats.m_uiAllocationSize, uiExpectedSize * 2);
  EZ_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 2);
#endif

  EZ_DELETE_ARRAY(pAllocator, TestArray);
  EZ_DELETE_RAW_BUFFER(pAllocator, pTestBuffer);

#if EZ_ENABLED(EZ_USE_ALLOCATION_TRACKING)
  stats = pAllocator->GetStats();
  EZ_TEST_INT(stats.m_uiAllocationSize, 0);
  EZ_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 0);
#endif
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

    ezLargeBlockAllocator<BLOCK_SIZE_IN_BYTES> allocator("Test", ezFoundation::GetDefaultAllocator());

    ezDynamicArray<ezDataBlock<int, BLOCK_SIZE_IN_BYTES>> blocks;
    blocks.Reserve(1000);

    for (ezUInt32 i = 0; i < 17; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      EZ_TEST_BOOL(ezMemoryUtils::IsAligned(block.m_pData, uiPageSize)); // test page alignment
      EZ_TEST_INT(block.m_uiCount, 0);

      blocks.PushBack(block);
    }

    ezAllocatorBase::Stats stats = allocator.GetStats();

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StackAllocator")
  {
    ezStackAllocator<> allocator("TestStackAllocator", ezFoundation::GetAlignedAllocator());

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StackAllocator with non-PODs")
  {
    ezStackAllocator<> allocator("TestStackAllocator", ezFoundation::GetAlignedAllocator());

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
}
