#include <RendererTest/RendererTestPCH.h>

#include "DynamicTextureAtlasTest.h"
#include <Foundation/SimdMath/SimdRandom.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

static ezGameEngineTestDynamicTextureAtlas g_DynamicTextureAtlasTest;

const char* ezGameEngineTestDynamicTextureAtlas::GetTestName() const
{
  return "DynamicTextureAtlas Tests";
}

ezGameEngineTestApplication* ezGameEngineTestDynamicTextureAtlas::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "DynamicTextureAtlas");
  return m_pOwnApplication;
}

void ezGameEngineTestDynamicTextureAtlas::SetupSubTests()
{
  AddSubTest("Allocations (Small)", SubTests::ST_AllocationsSmall);
  AddSubTest("Allocations (Large)", SubTests::ST_AllocationsLarge);
  AddSubTest("Allocations (Mixed)", SubTests::ST_AllocationsMixed);
  AddSubTest("Deallocations", SubTests::ST_Deallocations);
  AddSubTest("Deallocations 2", SubTests::ST_Deallocations2);
}

ezResult ezGameEngineTestDynamicTextureAtlas::InitializeSubTest(ezInt32 iIdentifier)
{
  struct AllocInfo
  {
    ezUInt32 m_uiWidth;
    ezUInt32 m_uiHeight;
    const char* m_szName;
  };

  m_iFrame = -1;

  ezGALTextureCreationDescription desc;
  desc.m_Format = ezGALResourceFormat::RUByteNormalized;
  desc.m_ResourceAccess.m_bImmutable = false;

  if (iIdentifier == SubTests::ST_AllocationsSmall)
  {
    {
      desc.m_uiWidth = 512 + 256;
      desc.m_uiHeight = 512;

      EZ_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr ezUInt32 uiSize = 32;

    ezStringBuilder sb;
    const ezUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);
    for (ezUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      EZ_TEST_BOOL(!id.IsInvalidated());
    }

    // Should be full now
    {
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, "Invalid");
      EZ_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_AllocationsLarge)
  {
    {
      desc.m_uiWidth = 512 + 256;
      desc.m_uiHeight = 512 + 256;

      EZ_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr ezUInt32 uiSize = 256;

    ezStringBuilder sb;
    const ezUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);
    for (ezUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      EZ_TEST_BOOL(!id.IsInvalidated());
    }

    // Should be full now
    {
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, "Invalid");
      EZ_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_AllocationsMixed)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      EZ_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    {
      auto id = m_TextureAtlas.Allocate(1024, 256, "Invalid");
      EZ_TEST_BOOL(id.IsInvalidated());
    }

    AllocInfo allocInfos[] = {
      {256, 256, "L1"},
      {128, 128, "M1"},
      {128, 128, "M2"},
      {128, 128, "M3"},
      {128, 64, "HM"},
      {64, 128, "VM"},
      {64, 64, "S1"},
      {64, 64, "S2"},
      {64, 64, "S3"},
      {64, 256, "VVS"},
      {256, 256, "L2"},
    };

    for (auto& a : allocInfos)
    {
      auto id = m_TextureAtlas.Allocate(a.m_uiWidth, a.m_uiHeight, a.m_szName);
      EZ_TEST_BOOL(!id.IsInvalidated());
    }

    {
      auto id = m_TextureAtlas.Allocate(256, 256, "Invalid");
      EZ_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_Deallocations)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      EZ_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr ezUInt32 uiSize = 128;

    ezStringBuilder sb;
    const ezUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);

    ezDynamicArray<ezDynamicTextureAtlas::AllocationId> allocations;
    allocations.Reserve(uiNumAllocations);

    for (ezUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      EZ_TEST_BOOL(!id.IsInvalidated());
      allocations.PushBack(id);
    }

    const ezUInt32 uiNumDeallocations = uiNumAllocations;

    for (ezUInt32 i = 0; i < uiNumDeallocations; ++i)
    {
      ezUInt32 allocationIndex = ezSimdRandom::UInt(ezSimdVec4i(i)).x() % allocations.GetCount();
      auto& id = allocations[allocationIndex];

      m_TextureAtlas.Deallocate(id);
      EZ_TEST_BOOL(id.IsInvalidated());

      allocations.RemoveAtAndSwap(allocationIndex);
    }

    // Atlas should be empty again at this point
    auto id = m_TextureAtlas.Allocate(512, 512, "A2");
    EZ_TEST_BOOL(!id.IsInvalidated());
  }
  else if (iIdentifier == SubTests::ST_Deallocations2)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      EZ_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr ezUInt32 uiSize = 32;

    ezStringBuilder sb;
    const ezUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);

    ezDynamicArray<ezDynamicTextureAtlas::AllocationId> allocations;
    allocations.Reserve(uiNumAllocations);

    for (ezUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      EZ_TEST_BOOL(!id.IsInvalidated());
      allocations.PushBack(id);
    }

    const ezUInt32 uiNumDeallocations = uiNumAllocations / 2;

    for (ezUInt32 i = 0; i < uiNumDeallocations; ++i)
    {
      ezUInt32 allocationIndex = ezSimdRandom::UInt(ezSimdVec4i(i)).x() % allocations.GetCount();
      auto& id = allocations[allocationIndex];

      m_TextureAtlas.Deallocate(id);
      EZ_TEST_BOOL(id.IsInvalidated());

      allocations.RemoveAtAndSwap(allocationIndex);
    }

    AllocInfo allocInfos[] = {
      {64, 32, "N1"},
      {64, 32, "N2"},
      {32, 64, "N3"},
      {128, 32, "N4"},
    };

    for (auto& a : allocInfos)
    {
      auto id = m_TextureAtlas.Allocate(a.m_uiWidth, a.m_uiHeight, a.m_szName);
      EZ_TEST_BOOL(!id.IsInvalidated());
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGameEngineTestDynamicTextureAtlas::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_TextureAtlas.Deinitialize();

  return EZ_SUCCESS;
}

ezTestAppRun ezGameEngineTestDynamicTextureAtlas::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView);
  auto& viewport = pView->GetViewport();

  m_TextureAtlas.DebugDraw(0, viewport.width, viewport.height);

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return ezTestAppRun::Quit;
  }

  if (m_iFrame == 1)
  {
    EZ_TEST_IMAGE(0, bVulkan ? 300 : 250);

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
