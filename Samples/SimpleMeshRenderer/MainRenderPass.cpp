#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RendererCore.h>

#include "MainRenderPass.h"

MainRenderPass::MainRenderPass() : ezRenderPipelinePass("MainRenderPass")
{
  m_pMeshRenderer = EZ_DEFAULT_NEW(ezMeshRenderer);
  AddRenderer(m_pMeshRenderer);
}

MainRenderPass::~MainRenderPass()
{
  EZ_DEFAULT_DELETE(m_pMeshRenderer);
}

void MainRenderPass::Execute(const ezRenderContext& renderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pContext = renderContext.m_pRenderer->GetGALContext();

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(pDevice->GetPrimarySwapChain());

  pContext->SetRenderTargetConfig(pSwapChain->GetRenderTargetViewConfig());
  pContext->Clear(ezColor::SlateGray);

  RenderDataWithPassType(renderContext, ezDefaultPassTypes::Opaque);
}
