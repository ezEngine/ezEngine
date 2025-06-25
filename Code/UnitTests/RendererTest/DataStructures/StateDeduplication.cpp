#include <RendererTest/RendererTestPCH.h>

#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/State/GraphicsPipeline.h>
#include <RendererFoundation/State/State.h>
#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

namespace
{
  void VerifyDependenciesExist(ezGALDevice* pDevice, const ezGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    EZ_TEST_BOOL(pDevice->GetShader(graphicsPipelineDesc.m_hShader) != nullptr);
    EZ_TEST_BOOL(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState) != nullptr);
    EZ_TEST_BOOL(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState) != nullptr);
    EZ_TEST_BOOL(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState) != nullptr);
  }

  void VerifyDependenciesAreDestroyed(ezGALDevice* pDevice, const ezGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    EZ_TEST_BOOL(pDevice->GetShader(graphicsPipelineDesc.m_hShader) == nullptr);
    EZ_TEST_BOOL(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState) == nullptr);
    EZ_TEST_BOOL(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState) == nullptr);
    EZ_TEST_BOOL(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState) == nullptr);
  }

  void VerifyDependenciesRefCount(ezGALDevice* pDevice, const ezGALGraphicsPipelineCreationDescription& graphicsPipelineDesc, ezUInt32 uiRefCount)
  {
    EZ_TEST_INT(pDevice->GetShader(graphicsPipelineDesc.m_hShader)->GetRefCount(), uiRefCount);
    EZ_TEST_INT(pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState)->GetRefCount(), uiRefCount);
    EZ_TEST_INT(pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState)->GetRefCount(), uiRefCount);
    EZ_TEST_INT(pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState)->GetRefCount(), uiRefCount);
  }

  void DestroyDependencies(ezGALDevice* pDevice, const ezGALGraphicsPipelineCreationDescription& graphicsPipelineDesc)
  {
    pDevice->DestroyShader(graphicsPipelineDesc.m_hShader);
    pDevice->DestroyBlendState(graphicsPipelineDesc.m_hBlendState);
    pDevice->DestroyDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState);
    pDevice->DestroyRasterizerState(graphicsPipelineDesc.m_hRasterizerState);
  }
} // namespace

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, StateDeduplication)



{
  ezGALShaderCreationDescription shaderDesc;
  ezGALBlendStateCreationDescription blendDesc;
  ezGALRasterizerStateCreationDescription rasterDesc;
  ezGALDepthStencilStateCreationDescription depthDesc;
  ezGALRenderPassDescriptor renderPassDesc;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto CreateDependencies = [&](ezGALGraphicsPipelineCreationDescription& ref_graphicsPipelineDesc)
  {
    ref_graphicsPipelineDesc.m_hShader = pDevice->CreateShader(shaderDesc);
    ref_graphicsPipelineDesc.m_hBlendState = pDevice->CreateBlendState(blendDesc);
    ref_graphicsPipelineDesc.m_hDepthStencilState = pDevice->CreateDepthStencilState(depthDesc);
    ref_graphicsPipelineDesc.m_hRasterizerState = pDevice->CreateRasterizerState(rasterDesc);
    ref_graphicsPipelineDesc.m_RenderPass = renderPassDesc;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LoadAndFreeTestData")
  {
    renderPassDesc.m_uiRTCount = 1;
    renderPassDesc.m_DepthFormat = ezGALResourceFormat::D24S8;
    renderPassDesc.m_ColorFormat[0] = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    renderPassDesc.m_ColorLoadOp[0] = ezGALRenderTargetLoadOp::Clear;
    renderPassDesc.m_ColorStoreOp[0] = ezGALRenderTargetStoreOp::Store;

    ezGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    ezGALGraphicsPipelineHandle hGraphicsPipeline;
    {
      ezShaderResourceHandle hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/MostBasicTriangle.ezShader");

      ezHashedString sFalse = ezMakeHashedString("FALSE");
      ezHashTable<ezHashedString, ezHashedString> permutationVariables;
      permutationVariables[ezMakeHashedString("CLIP_SPACE_FLIPPED")] = sFalse;
      permutationVariables[ezMakeHashedString("MSAA")] = sFalse;
      permutationVariables[ezMakeHashedString("TOPOLOGY")] = ezMakeHashedString("TOPOLOGY_TRIANGLES");

      ezShaderPermutationResourceHandle hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(hShader, permutationVariables, false);

      ezResourceLock<ezShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);

      graphicsPipelineDesc.m_hShader = pShaderPermutation->GetGALShader();
      graphicsPipelineDesc.m_hBlendState = pShaderPermutation->GetBlendState();
      graphicsPipelineDesc.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
      graphicsPipelineDesc.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
      graphicsPipelineDesc.m_RenderPass = renderPassDesc;

      shaderDesc = pDevice->GetShader(graphicsPipelineDesc.m_hShader)->GetDescription();
      blendDesc = pDevice->GetBlendState(graphicsPipelineDesc.m_hBlendState)->GetDescription();
      rasterDesc = pDevice->GetRasterizerState(graphicsPipelineDesc.m_hRasterizerState)->GetDescription();
      depthDesc = pDevice->GetDepthStencilState(graphicsPipelineDesc.m_hDepthStencilState)->GetDescription();

      hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);
    }

    ezUInt32 uiUnloaded = 0;
    for (ezUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += ezResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == 2)
        break;

      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    }
    EZ_TEST_INT_MSG(uiUnloaded, 2, "This should have freed the ezShaderResource and the ezShaderPermutationResource");

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    VerifyDependenciesExist(pDevice, graphicsPipelineDesc);

    // This will mark the pipeline and its dependencies as dead objects which will be freed in the next frame loop.
    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    EZ_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReferenceCounting")
  {
    ezGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    ezGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);

    VerifyDependenciesExist(pDevice, graphicsPipelineDesc);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);

    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    EZ_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReviveDeadObjects")
  {
    ezGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    ezGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    // This should revive the dead objects
    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    EZ_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "OddDestructionOrder")
  {
    ezGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    ezGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    // Destroy dependencies first this time
    DestroyDependencies(pDevice, graphicsPipelineDesc);
    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    // This should revive the dead objects
    CreateDependencies(graphicsPipelineDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    // Destroy the dependencies only partially
    pDevice->DestroyShader(graphicsPipelineDesc.m_hShader);
    pDevice->DestroyBlendState(graphicsPipelineDesc.m_hBlendState);
    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    graphicsPipelineDesc.m_hShader = pDevice->CreateShader(shaderDesc);
    graphicsPipelineDesc.m_hBlendState = pDevice->CreateBlendState(blendDesc);
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 2);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);
    DestroyDependencies(pDevice, graphicsPipelineDesc);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    EZ_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReviveDeadDependencies")
  {
    ezGALGraphicsPipelineCreationDescription graphicsPipelineDesc;
    ezGALGraphicsPipelineHandle hGraphicsPipeline;

    CreateDependencies(graphicsPipelineDesc);
    DestroyDependencies(pDevice, graphicsPipelineDesc);
    // Now this is technically invalid code as we are creating a pipeline referencing dead handles. However, this works for all GAL state objects right now and is a side-effect of the fact that objects are deleted at the end of the frame.
    hGraphicsPipeline = pDevice->CreateGraphicsPipeline(graphicsPipelineDesc);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 1);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 1);

    pDevice->DestroyGraphicsPipeline(hGraphicsPipeline);

    EZ_TEST_INT(pDevice->GetGraphicsPipeline(hGraphicsPipeline)->GetRefCount(), 0);
    VerifyDependenciesRefCount(pDevice, graphicsPipelineDesc, 0);

    pDevice->BeginFrame(1);
    pDevice->EndFrame();

    EZ_TEST_BOOL(pDevice->GetGraphicsPipeline(hGraphicsPipeline) == nullptr);
    VerifyDependenciesAreDestroyed(pDevice, graphicsPipelineDesc);
  }
}
