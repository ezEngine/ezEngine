#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>

#include "MainRenderPass.h"

MainRenderPass::MainRenderPass() : ezRenderPipelinePass("MainRenderPass")
{
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
}

void MainRenderPass::Execute(const ezRenderContext& renderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = renderContext.m_pGALContext;

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(pDevice->GetPrimarySwapChain());

  pContext->SetRenderTargetConfig(pSwapChain->GetRenderTargetViewConfig());
  pContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  RenderDataWithPassType(renderContext, ezDefaultPassTypes::Opaque);
}
