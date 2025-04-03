#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/Basics/DynamicBufferTest.h>

#include <RendererFoundation/Resources/DynamicBuffer.h>

void ezRendererTestDynamicBuffer::SetupSubTests()
{
  AddSubTest("Allocations", SubTests::ST_Allocations);
  AddSubTest("Deallocations", SubTests::ST_Deallocations);
  AddSubTest("Compaction", SubTests::ST_Compaction);
}

ezResult ezRendererTestDynamicBuffer::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezUInt64);
  desc.m_uiTotalSize = desc.m_uiStructSize * 16;
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;

  m_hDynamicBuffer = ezGALDevice::GetDefaultDevice()->CreateDynamicBuffer(desc, "Test buffer");

  return EZ_SUCCESS;
}

ezResult ezRendererTestDynamicBuffer::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ezGALDevice::GetDefaultDevice()->DestroyDynamicBuffer(m_hDynamicBuffer);

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestDynamicBuffer::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::ST_Allocations)
  {
    constexpr ezUInt32 uiAllocationSizes[] = {11, 23, 4, 5, 6, 7, 8, 9};

    auto pDevice = ezGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    ezUInt32 uiTotalOffset = 0;
    auto AllocateAndMap = [&](ezUInt32 uiIndex)
    {
      ezUInt32 uiOffset = pDynamicBuffer->Allocate(uiIndex, uiAllocationSizes[uiIndex]);
      EZ_TEST_INT(uiOffset, uiTotalOffset);

      auto pMappedData = pDynamicBuffer->MapForWriting<ezUInt64>(uiOffset);
      EZ_TEST_INT(pMappedData.GetCount(), uiAllocationSizes[uiIndex]);

      uiTotalOffset += uiAllocationSizes[uiIndex];
    };

    AllocateAndMap(0);

    pDynamicBuffer->UploadChangesForNextFrame();

    // Internal buffer should have been created after upload but GetBufferForRendering should still return the old handle until the next frame.
    ezGALBufferHandle hBuffer = pDynamicBuffer->GetBufferForRendering();
    EZ_TEST_BOOL(hBuffer.IsInvalidated());

    {
      BeginFrame();

      // After one frame the buffer should be created
      ezGALBufferHandle hNewBuffer = pDynamicBuffer->GetBufferForRendering();
      EZ_TEST_BOOL(hNewBuffer.IsInvalidated() == false);
      hBuffer = hNewBuffer;

      EndFrame();
    }

    for (ezUInt32 i = 1; i < EZ_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      AllocateAndMap(i);
    }

    pDynamicBuffer->UploadChangesForNextFrame();

    ezGALBufferHandle hNewBuffer = pDynamicBuffer->GetBufferForRendering();
    EZ_TEST_BOOL(hNewBuffer.IsInvalidated() == false);

    // The internal buffer has been re-allocated but GetBufferForRendering should still return the old handle until the next frame.
    EZ_TEST_BOOL(hNewBuffer == hBuffer);

    {
      BeginFrame();

      // Now the new buffer should be returned
      hNewBuffer = pDynamicBuffer->GetBufferForRendering();
      EZ_TEST_BOOL(hNewBuffer != hBuffer);

      EndFrame();
    }
  }
  else if (iIdentifier == SubTests::ST_Deallocations)
  {
    constexpr ezUInt32 uiAllocationSizes[] = {3, 4, 5, 6, 7, 9, 11, 13};

    auto pDevice = ezGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    ezHybridArray<ezUInt32, 16> offsets;
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    // Create some holes out of order
    pDynamicBuffer->Deallocate(offsets[1]);
    pDynamicBuffer->Deallocate(offsets[5]);
    pDynamicBuffer->Deallocate(offsets[3]);

    // Should try to waste as little space as possible so this allocation should go into the middle hole with size 6.
    ezUInt32 uiNewOffset = pDynamicBuffer->Allocate(100, 5);
    EZ_TEST_INT(uiNewOffset, offsets[3]);

    pDynamicBuffer->Deallocate(uiNewOffset);

    // Should merge all holes into one big hole
    pDynamicBuffer->Deallocate(offsets[4]);
    pDynamicBuffer->Deallocate(offsets[2]);

    // Test whether the merging was done correctly by allocation something almost as big as the hole
    uiNewOffset = pDynamicBuffer->Allocate(101, 30);
    EZ_TEST_INT(uiNewOffset, offsets[1]);

    // Deallocate the last allocation
    pDynamicBuffer->Deallocate(offsets[7]);

    uiNewOffset = pDynamicBuffer->Allocate(102, 100);
    EZ_TEST_INT(uiNewOffset, offsets[7]);
  }
  else if (iIdentifier == SubTests::ST_Compaction)
  {
    constexpr ezUInt32 uiAllocationSizes[] = {15, 14, 13, 11, 7, 9, 15, 14};

    auto pDevice = ezGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    ezHybridArray<ezUInt32, 16> offsets;
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    // Create some holes out of order
    pDynamicBuffer->Deallocate(offsets[5]);
    pDynamicBuffer->Deallocate(offsets[1]);
    pDynamicBuffer->Deallocate(offsets[3]);

    ezHybridArray<ezGALDynamicBuffer::ChangedAllocation, 16> changedAllocations;

    // The first compaction step should move the last allocation into the first hole
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 1);

      EZ_TEST_INT(changedAllocations.GetCount(), 1);
      EZ_TEST_INT(changedAllocations[0].m_uiUserData, 7);
      EZ_TEST_INT(changedAllocations[0].m_uiNewOffset, offsets[1]);
    }

    // For the next steps the last allocation does not match the hole size so it should start to move allocations forward to close the hole
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 16);

      EZ_TEST_INT(changedAllocations.GetCount(), 2);
      EZ_TEST_INT(changedAllocations[0].m_uiUserData, 4);
      EZ_TEST_INT(changedAllocations[0].m_uiNewOffset, offsets[3]);
      EZ_TEST_INT(changedAllocations[1].m_uiUserData, 6);
      EZ_TEST_INT(changedAllocations[1].m_uiNewOffset, offsets[3] + uiAllocationSizes[4]);
    }

    pDynamicBuffer->Clear();
    offsets.Clear();

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    pDynamicBuffer->Deallocate(offsets[6]);
    pDynamicBuffer->Deallocate(offsets[7]);

    // Compaction should have already happened in deallocate
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 16);
      EZ_TEST_INT(changedAllocations.GetCount(), 0);
    }

    ezUInt32 uiOffset = pDynamicBuffer->Allocate(400, 20);
    EZ_TEST_INT(uiOffset, offsets[6]);
  }

  return ezTestAppRun::Quit;
}

static ezRendererTestDynamicBuffer g_DynamicBufferTest;
