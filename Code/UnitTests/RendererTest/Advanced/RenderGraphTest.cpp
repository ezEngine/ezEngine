#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererTest/Advanced/RenderGraphTest.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>

static ezRenderGraphTest s_RenderGraphTest;

void ezRenderGraphTest::SetupSubTests()
{
  AddSubTest("DeadPassCulling", SubTests::ST_DeadPassCulling);
  AddSubTest("ResourceAliasing", SubTests::ST_ResourceAliasing);
  AddSubTest("ImportReplace", SubTests::ST_ImportReplace);
  AddSubTest("ExecuteCallbacks", SubTests::ST_ExecuteCallbacks);
  AddSubTest("EmptyGraph", SubTests::ST_EmptyGraph);
  AddSubTest("StressTest", SubTests::ST_StressTest);
  AddSubTest("MsaaResolve", SubTests::ST_MsaaResolve);
}

ezResult ezRenderGraphTest::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow());

  ezGPUResourcePool* pResourcePool = EZ_DEFAULT_NEW(ezGPUResourcePool);
  ezGPUResourcePool::SetDefaultInstance(pResourcePool);
  m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("RendererTest");

  return EZ_SUCCESS;
}

ezResult ezRenderGraphTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_pRenderGraph = nullptr;
  ezGPUResourcePool::SetDefaultInstance(nullptr);
  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRenderGraphTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;

  switch (iIdentifier)
  {
    case SubTests::ST_DeadPassCulling:
      DeadPassCulling();
      return ezTestAppRun::Quit;
    case SubTests::ST_ResourceAliasing:
      ResourceAliasing();
      return ezTestAppRun::Quit;
    case SubTests::ST_ImportReplace:
      ImportReplace();
      return ezTestAppRun::Quit;
    case SubTests::ST_ExecuteCallbacks:
      ExecuteCallbacks();
      return ezTestAppRun::Quit;
    case SubTests::ST_EmptyGraph:
      EmptyGraph();
      return ezTestAppRun::Quit;
    case SubTests::ST_StressTest:
      return StressTestRenderGraph(256);
    case SubTests::ST_MsaaResolve:
      MsaaResolve();
      return ezTestAppRun::Quit;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezTestAppRun::Quit;
  }
}

// ============================================================
// Helper: create a simple render target texture description
// ============================================================

namespace
{
  ezGALTextureCreationDescription MakeColorRT(ezUInt32 w = 64, ezUInt32 h = 64)
  {
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(w, h, ezGALResourceFormat::RGBAHalf);
    return desc;
  }

  auto NoopCallback()
  {
    return [](const ezRenderGraphContext&) {};
  }
} // namespace

// ============================================================
// Test: Dead Pass Culling
// ============================================================

void ezRenderGraphTest::DeadPassCulling()
{
  BeginFrame();

  // Build a graph with a mix of side-effecting and non-side-effecting passes.
  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();
  auto hTex = graph.CreateTexture(desc);

  // Pass 0: writes hTex, no side effects, no downstream consumer -> should be culled.
  bool bCulledPassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("ShouldBeCulled");
    pass.AddColorTarget(hTex, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, ezColor::Red);
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bCulledPassExecuted = true; });
  }

  // Pass 1: writes to a different texture, has side effects -> should survive.
  auto hTex2 = graph.CreateTexture(desc);
  bool bSideEffectPassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("HasSideEffects");
    pass.AddColorTarget(hTex2, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, ezColor::Blue);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bSideEffectPassExecuted = true; });
  }

  // Pass 2: writes to an imported texture -> should survive (implicit side effect).
  const ezGALSwapChain* pSwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
  auto hImported = graph.ImportTexture(pSwapChain->GetBackBufferTexture());
  bool bImportWritePassExecuted = false;
  {
    auto pass = graph.AddGraphicsPass("WritesToImported");
    pass.AddColorTarget(hImported, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0, ezColor::Green);
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bImportWritePassExecuted = true; });
  }

  // Pass 3: reads hTex but hTex's writer is culled. This pass has no side effects either -> should be culled.
  bool bReaderOfCulledExecuted = false;
  {
    auto pass = graph.AddComputePass("ReadsFromCulled");
    pass.ReadTexture(hTex, {}, ezGALResourceState::ShaderResource);
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bReaderOfCulledExecuted = true; });
  }

  // Pass 4: Transitive dependency chain: A->B->C where C has side effects.
  auto hChainTex1 = graph.CreateTexture(desc);
  auto hChainTex2 = graph.CreateTexture(desc);
  bool bChainAExecuted = false, bChainBExecuted = false, bChainCExecuted = false;
  {
    auto passA = graph.AddGraphicsPass("ChainA");
    passA.AddColorTarget(hChainTex1, {}, ezGALRenderTargetLoadOp::Clear);
    passA.SetClearColor(0, ezColor::White);
    passA.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bChainAExecuted = true; });
  }
  {
    auto passB = graph.AddGraphicsPass("ChainB");
    passB.ReadTexture(hChainTex1, {}, ezGALResourceState::ShaderResource);
    passB.AddColorTarget(hChainTex2, {}, ezGALRenderTargetLoadOp::Clear);
    passB.SetClearColor(0, ezColor::White);
    passB.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bChainBExecuted = true; });
  }
  {
    auto passC = graph.AddGraphicsPass("ChainC");
    passC.ReadTexture(hChainTex2, {}, ezGALResourceState::ShaderResource);
    passC.AddColorTarget(hChainTex2, {}, ezGALRenderTargetLoadOp::Clear);
    passC.HasSideEffects();
    passC.SetExecuteCallback([&](const ezRenderGraphContext&)
      { bChainCExecuted = true; });
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  EZ_TEST_BOOL_MSG(!bCulledPassExecuted, "Pass without side effects and no consumer should be culled");
  EZ_TEST_BOOL_MSG(bSideEffectPassExecuted, "Pass with HasSideEffects should survive");
  EZ_TEST_BOOL_MSG(bImportWritePassExecuted, "Pass writing to imported texture should survive");
  EZ_TEST_BOOL_MSG(!bReaderOfCulledExecuted, "Reader of a culled pass should also be culled");
  EZ_TEST_BOOL_MSG(bChainAExecuted, "Chain source should survive via transitive dependency");
  EZ_TEST_BOOL_MSG(bChainBExecuted, "Chain middle should survive via transitive dependency");
  EZ_TEST_BOOL_MSG(bChainCExecuted, "Chain sink with side effects should survive");
}

// ============================================================
// Test: Resource Aliasing
// ============================================================

void ezRenderGraphTest::ResourceAliasing()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();

  // Two textures with non-overlapping lifetimes and identical descriptions.
  // Pass 0 writes texA, Pass 1 reads texA (texA's lifetime ends).
  // Pass 2 writes texB, Pass 3 reads texB (texB's lifetime ends).
  // texA and texB should alias to the same GPU resource.
  auto hTexA = graph.CreateTexture(desc);
  auto hTexB = graph.CreateTexture(desc);

  ezGALTextureHandle hResolvedA, hResolvedB;

  {
    auto pass0 = graph.AddGraphicsPass("WriteA");
    pass0.AddColorTarget(hTexA, {}, ezGALRenderTargetLoadOp::Clear);
    pass0.SetClearColor(0, ezColor::Red);
    pass0.SetExecuteCallback([&](const ezRenderGraphContext& ctx)
      { hResolvedA = ctx.ResolveTexture(hTexA); });
  }
  {
    auto pass1 = graph.AddGraphicsPass("ReadA");
    pass1.ReadTexture(hTexA);
    pass1.HasSideEffects();
    pass1.SetExecuteCallback(NoopCallback());
  }
  {
    auto pass2 = graph.AddGraphicsPass("WriteB");
    pass2.AddColorTarget(hTexB, {}, ezGALRenderTargetLoadOp::Clear);
    pass2.SetClearColor(0, ezColor::Blue);
    pass2.SetExecuteCallback([&](const ezRenderGraphContext& ctx)
      { hResolvedB = ctx.ResolveTexture(hTexB); });
  }
  {
    auto pass3 = graph.AddGraphicsPass("ReadB");
    pass3.ReadTexture(hTexB);
    pass3.HasSideEffects();
    pass3.SetExecuteCallback(NoopCallback());
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  EZ_TEST_BOOL_MSG(!hResolvedA.IsInvalidated(), "texA should resolve to a valid GPU handle");
  EZ_TEST_BOOL_MSG(!hResolvedB.IsInvalidated(), "texB should resolve to a valid GPU handle");
  EZ_TEST_BOOL_MSG(hResolvedA == hResolvedB, "Non-overlapping same-desc textures should alias");

  // Now test overlapping lifetimes: texC and texD are both alive during pass1.
  BeginFrame();
  graph.Reset();

  auto hTexC = graph.CreateTexture(desc);
  auto hTexD = graph.CreateTexture(desc);

  ezGALTextureHandle hResolvedC, hResolvedD;

  {
    auto pass0 = graph.AddGraphicsPass("WriteC");
    pass0.AddColorTarget(hTexC, {}, ezGALRenderTargetLoadOp::Clear);
    pass0.SetClearColor(0);
    pass0.SetExecuteCallback([&](const ezRenderGraphContext& ctx)
      { hResolvedC = ctx.ResolveTexture(hTexC); });
  }
  {
    auto pass1 = graph.AddGraphicsPass("WriteDReadC");
    pass1.ReadTexture(hTexC);
    pass1.AddColorTarget(hTexD, {}, ezGALRenderTargetLoadOp::Clear);
    pass1.SetClearColor(0);
    pass1.SetExecuteCallback([&](const ezRenderGraphContext& ctx)
      { hResolvedD = ctx.ResolveTexture(hTexD); });
  }
  {
    auto pass2 = graph.AddGraphicsPass("ReadD");
    pass2.ReadTexture(hTexD);
    pass2.HasSideEffects();
    pass2.SetExecuteCallback(NoopCallback());
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  EZ_TEST_BOOL_MSG(hResolvedC != hResolvedD, "Overlapping-lifetime textures must not alias");
}

// ============================================================
// Test: Import / Replace
// ============================================================

void ezRenderGraphTest::ImportReplace()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  // Create three GPU textures: A and B have matching descriptions, C has a different format.
  ezGALTextureCreationDescription texDesc;
  texDesc.SetAsRenderTarget(64, 64, ezGALResourceFormat::RGBAHalf);
  ezGALTextureHandle hGPUTexA = m_pDevice->CreateTexture(texDesc);
  ezGALTextureHandle hGPUTexB = m_pDevice->CreateTexture(texDesc);
  EZ_SCOPE_EXIT(m_pDevice->DestroyTexture(hGPUTexA); m_pDevice->DestroyTexture(hGPUTexB));

  ezGALTextureCreationDescription texDescDiff;
  texDescDiff.SetAsRenderTarget(64, 64, ezGALResourceFormat::RFloat);
  ezGALTextureHandle hGPUTexDiff = m_pDevice->CreateTexture(texDescDiff);
  EZ_SCOPE_EXIT(m_pDevice->DestroyTexture(hGPUTexDiff));

  // Import returns the same handle for the same GPU texture.
  auto hImportA = graph.ImportTexture(hGPUTexA);
  auto hImportA2 = graph.ImportTexture(hGPUTexA);
  EZ_TEST_BOOL(hImportA == hImportA2);

  // Add a side-effect pass so the graph compiles.
  {
    auto pass = graph.AddGraphicsPass("Use");
    pass.AddColorTarget(hImportA, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback(NoopCallback());
  }

  // Replace A with B (matching description, B not yet imported) -> should succeed.
  EZ_TEST_BOOL(graph.ReplaceImportedTexture(hImportA, hGPUTexB).Succeeded());

  // Replace A (now pointing to B) with a different format -> should fail.
  EZ_TEST_BOOL(graph.ReplaceImportedTexture(hImportA, hGPUTexDiff).Failed());

  // Compile so that resolved textures are available.
  EZ_TEST_RESULT(graph.Compile());

  // After replace, re-compiling and executing should use the new texture (B).
  ezGALTextureHandle hResolved;
  graph.Reset();
  auto hImportNew = graph.ImportTexture(hGPUTexB);
  {
    auto pass = graph.AddGraphicsPass("VerifyReplace");
    pass.AddColorTarget(hImportNew, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const ezRenderGraphContext& ctx)
      { hResolved = ctx.ResolveTexture(hImportNew); });
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  EZ_TEST_BOOL(hResolved == hGPUTexB);
}

// ============================================================
// Test: Execute Callbacks
// ============================================================

void ezRenderGraphTest::ExecuteCallbacks()
{
  BeginFrame();

  auto& graph = *m_pRenderGraph;
  graph.Reset();

  auto desc = MakeColorRT();

  // Track execution order via a list of IDs.
  ezDynamicArray<int> executionOrder;

  auto hTex = graph.CreateTexture(desc);

  // Pass 0: graphics, side effect
  {
    auto pass = graph.AddGraphicsPass("Pass0");
    pass.AddColorTarget(hTex, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { executionOrder.PushBack(0); });
  }

  // Pass 1: compute, reads tex from pass 0
  {
    auto pass = graph.AddComputePass("Pass1");
    pass.ReadTexture(hTex, {}, ezGALResourceState::ShaderResource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { executionOrder.PushBack(1); });
  }

  // Pass 2: should be culled (no side effects, no consumer)
  auto hDeadTex = graph.CreateTexture(desc);
  {
    auto pass = graph.AddGraphicsPass("Pass2-Dead");
    pass.AddColorTarget(hDeadTex, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearColor(0);
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { executionOrder.PushBack(2); });
  }

  // Pass 3: transfer, side effect
  {
    auto pass = graph.AddTransferPass("Pass3");
    pass.HasSideEffects();
    pass.SetExecuteCallback([&](const ezRenderGraphContext&)
      { executionOrder.PushBack(3); });
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  // Pass2 should be culled. Remaining passes should execute in declaration order.
  EZ_TEST_INT(executionOrder.GetCount(), 3);
  if (executionOrder.GetCount() == 3)
  {
    EZ_TEST_INT(executionOrder[0], 0);
    EZ_TEST_INT(executionOrder[1], 1);
    EZ_TEST_INT(executionOrder[2], 3);
  }
}

// ============================================================
// Test: Empty Graph
// ============================================================

void ezRenderGraphTest::EmptyGraph()
{
  // A graph with no passes should compile and execute without error.
  {
    BeginFrame();
    auto& graph = *m_pRenderGraph;
    graph.Reset();
    ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
    ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
    EndFrame();
  }

  // A graph where ALL passes are culled should also work.
  {
    BeginFrame();
    auto& graph = *m_pRenderGraph;
    graph.Reset();

    auto desc = MakeColorRT();
    auto hTex = graph.CreateTexture(desc);

    {
      auto pass = graph.AddGraphicsPass("AllCulled");
      pass.AddColorTarget(hTex, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0);
      // No HasSideEffects, no downstream consumer -> culled.
      pass.SetExecuteCallback(NoopCallback());
    }
    ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
    ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
    EndFrame();
  }
}

// ============================================================
// Test: MSAA Resolve
// ============================================================

void ezRenderGraphTest::MsaaResolve()
{
  BeginFrame();

  ezRenderGraph& graph = *m_pRenderGraph.Borrow();
  graph.Reset();

  // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-create-resources
  // Create transient MSAA texture
  ezGALTextureCreationDescription descMsaa;
  descMsaa.SetAsRenderTarget(64, 64, ezGALResourceFormat::RGBAHalf, ezGALMSAASampleCount::FourSamples);
  ezRenderGraphTextureHandle hMsaaColor = graph.CreateTexture(descMsaa);

  // Import persistent non-MSAA resolve target
  ezGALTextureCreationDescription descResolved;
  descResolved.SetAsRenderTarget(64, 64, ezGALResourceFormat::RGBAHalf);
  ezGALTextureHandle hResolvedGAL = m_pDevice->CreateTexture(descResolved);
  ezRenderGraphTextureHandle hResolved = graph.ImportTexture(hResolvedGAL);
  // END-DOCS-CODE-SNIPPET

  // Create transient MSAA depth texture
  ezGALTextureCreationDescription descMsaaDepth;
  descMsaaDepth.SetAsRenderTarget(64, 64, ezGALResourceFormat::D24S8, ezGALMSAASampleCount::FourSamples);
  ezRenderGraphTextureHandle hMsaaDepth = graph.CreateTexture(descMsaaDepth);

  // Render into the MSAA target.
  {
    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-graphics-pass
    auto pass = graph.AddGraphicsPass("RenderMSAA");
    pass.AddColorTarget(hMsaaColor, {}, ezGALRenderTargetLoadOp::Clear, ezGALRenderTargetStoreOp::Store);
    pass.SetClearColor(0, ezColor::CornflowerBlue);
    pass.AddDepthStencilTarget(hMsaaDepth, {}, ezGALRenderTargetLoadOp::Clear, ezGALRenderTargetStoreOp::Store, ezGALRenderTargetLoadOp::Clear, ezGALRenderTargetStoreOp::Store);
    pass.SetClearDepth(1.0f);
    pass.SetClearStencil(0);
    pass.SetStereoscopic(false);
    pass.HasSideEffects();
    // END-DOCS-CODE-SNIPPET
  }

  // Resolve the MSAA texture into the imported target.
  {
    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-barriers
    auto pass = graph.AddTransferPass("MsaaColorResolve");
    pass.ReadTexture(hMsaaColor, {}, ezGALResourceState::ResolveSource);
    pass.WriteTexture(hResolved, {}, ezGALResourceState::ResolveDestination);
    // END-DOCS-CODE-SNIPPET

    // BEGIN-DOCS-CODE-SNIPPET: rendergraph-msaa-execute-callback
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      ezGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hResolved), subresource, ctx.ResolveTexture(hMsaaColor), subresource); });
    // END-DOCS-CODE-SNIPPET
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  m_pDevice->DestroyTexture(hResolvedGAL);
}

// ============================================================
// Test: Stress Test (kept from original)
// ============================================================

ezTestAppRun ezRenderGraphTest::StressTestRenderGraph(ezUInt32 uiNumPasses)
{
  BeginFrame();

  ezRenderGraph& graph = *m_pRenderGraph;
  graph.Reset();

  EZ_PROFILE_SCOPE("StressTestRenderGraph");
  {
    EZ_PROFILE_SCOPE("BuildGraph");
    ezRandom rng;
    rng.Initialize(0x52454E44);

    const ezSizeU32 res = GetResolution();
    const ezUInt32 uiBaseWidth = res.width;
    const ezUInt32 uiBaseHeight = res.height;

    constexpr ezUInt32 uiNumTextures = 32;
    constexpr ezUInt32 uiNumBuffers = 8;

    ezGALResourceFormat::Enum textureFormats[] = {
      ezGALResourceFormat::RGBAHalf,
      ezGALResourceFormat::RGHalf,
      ezGALResourceFormat::RHalf,
      ezGALResourceFormat::RGBAFloat,
      ezGALResourceFormat::BGRAUByteNormalized,
      ezGALResourceFormat::RFloat,
    };

    ezStaticArray<ezRenderGraphTextureHandle, uiNumTextures> textures;
    for (ezUInt32 i = 0; i < uiNumTextures; ++i)
    {
      const ezUInt32 uiDivisor = 1u << rng.UIntInRange(3);
      const ezUInt32 w = ezMath::Max(1u, uiBaseWidth / uiDivisor);
      const ezUInt32 h = ezMath::Max(1u, uiBaseHeight / uiDivisor);
      const auto format = textureFormats[rng.UIntInRange(EZ_ARRAY_SIZE(textureFormats))];

      ezGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(w, h, format, ezGALMSAASampleCount::None);
      desc.m_TextureFlags.Add(ezGALTextureUsageFlags::UnorderedAccess);
      textures.PushBack(graph.CreateTexture(desc));
    }

    ezGALTextureCreationDescription depthDesc;
    depthDesc.SetAsRenderTarget(uiBaseWidth, uiBaseHeight, ezGALResourceFormat::D16, ezGALMSAASampleCount::None);
    ezRenderGraphTextureHandle hDepth = graph.CreateTexture(depthDesc);

    ezStaticArray<ezRenderGraphBufferHandle, uiNumBuffers> buffers;
    for (ezUInt32 i = 0; i < uiNumBuffers; ++i)
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = 4;
      desc.m_uiTotalSize = (128 + rng.UIntInRange(1024)) * 4;
      desc.m_BufferFlags = ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
      desc.m_ResourceAccess.m_bImmutable = false;
      buffers.PushBack(graph.CreateBuffer(desc));
    }

    const ezGALSwapChain* pSwapChain = m_pDevice->GetSwapChain(m_hSwapChain);
    ezRenderGraphTextureHandle hBackbuffer = graph.ImportTexture(pSwapChain->GetBackBufferTexture());

    auto emptyCallback = [](const ezRenderGraphContext&) {};

    for (ezUInt32 p = 0; p < uiNumPasses; ++p)
    {
      const ezUInt32 uiPassType = rng.UIntInRange(3);

      if (uiPassType == 0)
      {
        auto pass = graph.AddGraphicsPass("StressGraphics");
        const ezUInt32 uiReads = 1 + rng.UIntInRange(3);
        for (ezUInt32 r = 0; r < uiReads; ++r)
          pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)]);
        pass.AddColorTarget(textures[rng.UIntInRange(uiNumTextures)], {}, ezGALRenderTargetLoadOp::Clear);
        pass.SetClearColor(0, ezColor::Black);
        if (rng.Bool())
        {
          pass.AddDepthStencilTarget(hDepth);
          pass.SetClearDepth();
        }
        if (rng.Bool())
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], ezGALResourceState::ShaderResource);
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
      else if (uiPassType == 1)
      {
        auto pass = graph.AddComputePass("StressCompute");
        const ezUInt32 uiReads = 1 + rng.UIntInRange(2);
        for (ezUInt32 r = 0; r < uiReads; ++r)
          pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)], {}, ezGALResourceState::ShaderResource);
        pass.WriteTexture(textures[rng.UIntInRange(uiNumTextures)], {}, ezGALResourceState::UnorderedAccess);
        if (rng.Bool())
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], ezGALResourceState::ShaderResource);
        if (rng.Bool())
          pass.WriteBuffer(buffers[rng.UIntInRange(uiNumBuffers)]);
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
      else
      {
        auto pass = graph.AddTransferPass("StressTransfer");
        pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)], {}, ezGALResourceState::CopySource);
        pass.WriteTexture(textures[rng.UIntInRange(uiNumTextures)], {}, ezGALResourceState::CopyDestination);
        if (rng.Bool())
        {
          pass.ReadBuffer(buffers[rng.UIntInRange(uiNumBuffers)], ezGALResourceState::CopySource);
          pass.WriteBuffer(buffers[rng.UIntInRange(uiNumBuffers)], ezGALResourceState::CopyDestination);
        }
        pass.HasSideEffects();
        pass.SetExecuteCallback(emptyCallback);
      }
    }

    {
      auto pass = graph.AddGraphicsPass("StressFinalComposite");
      pass.AddColorTarget(hBackbuffer, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, ezColor::Black);
      for (ezUInt32 r = 0; r < 4; ++r)
        pass.ReadTexture(textures[rng.UIntInRange(uiNumTextures)]);
      pass.SetExecuteCallback(emptyCallback);
    }
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  ezRenderGraphManager::ExecuteRenderGraphs(m_pDevice);
  EndFrame();

  if (m_iFrame < 255)
    return ezTestAppRun::Continue;

  ezProfilingUtils::SaveProfilingCapture(":imgout/Profiling/profiling.json").IgnoreResult();
  return ezTestAppRun::Quit;
}
