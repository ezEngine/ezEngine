#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

#include <Core/ResourceManager/ResourceManager.h>

#include "MainRenderPass.h"

static ezShaderResourceHandle s_hShader;

MainRenderPass::MainRenderPass() : ezRenderPipelinePass("MainRenderPass")
{
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));

  // todo: shader should come from material
  s_hShader = ezResourceManager::GetResourceHandle<ezShaderResource>("Generic.shader");
}

void MainRenderPass::Execute()
{
  ezGALDevice* pDevice = GetPipeline()->GetDevice();
  ezGALContext* pContext = pDevice->GetPrimaryContext();

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(pDevice->GetPrimarySwapChain());

  pContext->SetRenderTargetConfig(pSwapChain->GetRenderTargetViewConfig());
  pContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  ezShaderManager::SetActiveShader(s_hShader);

  RenderDataWithPassType(ezDefaultPassTypes::Opaque);
}
