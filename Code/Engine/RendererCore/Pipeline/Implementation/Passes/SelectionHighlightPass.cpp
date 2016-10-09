#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionHighlightPass, 1, ezRTTIDefaultAllocator<ezSelectionHighlightPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSelectionHighlightPass::ezSelectionHighlightPass(const char* szName) : ezRenderPipelinePass(szName)
{
  ezGALSamplerStateCreationDescription desc;
  desc.m_MagFilter = ezGALTextureFilterMode::Point;
  desc.m_MinFilter = ezGALTextureFilterMode::Point;
  desc.m_MipFilter = ezGALTextureFilterMode::Point;
  desc.m_AddressU = ezGALTextureAddressMode::Border;
  desc.m_AddressV = ezGALTextureAddressMode::Border;
  desc.m_BorderColor = ezColor::White;
  m_hSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  

  // Load shader.
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SelectionHighlight.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");
}

ezSelectionHighlightPass::~ezSelectionHighlightPass()
{
  ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
}

bool ezSelectionHighlightPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
    return true;
  }

  return false;
}

void ezSelectionHighlightPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
  {
    return;
  }

  ezRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  pGALContext->SetViewport(renderViewContext.m_pViewData->m_ViewPortRect);

  ezGALTextureHandle hDepthTexture;

  // render all selection objects to depth target only
  {
    ezUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    ezUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = ezGALResourceFormat::D24S8;
    desc.m_bCreateRenderTarget = true;

    hDepthTexture = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hDepthTexture));

    pGALContext->SetRenderTargetSetup(renderTargetSetup);

    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "DEPTH_ONLY");

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
  }

  // reconstruct selection overlay from depth target
  {
    ezGALRenderTagetSetup renderTargetSetup;
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

    pGALContext->SetRenderTargetSetup(renderTargetSetup);

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SelectionDepthTexture", pDevice->GetDefaultResourceView(hDepthTexture));
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SceneDepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindSamplerState(ezGALShaderStage::PixelShader, "BorderClampSampler", m_hSamplerState);
    
    renderViewContext.m_pRenderContext->DrawMeshBuffer();

    // Prevent shader resource hazard in DX11
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SelectionDepthTexture", ezGALResourceViewHandle());
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "SceneDepthTexture", ezGALResourceViewHandle());
    renderViewContext.m_pRenderContext->ApplyContextStates();
    pGALContext->Flush();

    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hDepthTexture);
  }  
}
