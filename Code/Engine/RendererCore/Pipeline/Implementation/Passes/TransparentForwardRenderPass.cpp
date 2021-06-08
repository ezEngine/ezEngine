#include <RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransparentForwardRenderPass, 1, ezRTTIDefaultAllocator<ezTransparentForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTransparentForwardRenderPass::ezTransparentForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezTransparentForwardRenderPass::~ezTransparentForwardRenderPass()
{
  if (!m_hSceneColorSamplerState.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
    m_hSceneColorSamplerState.Invalidate();
  }
}

void ezTransparentForwardRenderPass::Execute(const ezRenderViewContext& renderViewContext,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
  {
    return;
  }

  CreateSamplerState();

  ezUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  ezUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;

  ezGALTextureCreationDescription desc;
  desc.SetAsRenderTarget(uiWidth, uiHeight, pColorInput->m_Desc.m_Format);
  desc.m_uiMipLevelCount = 1;

  ezGALTextureHandle hSceneColor = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALPass* pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  UpdateSceneColorTexture(renderViewContext, hSceneColor, pColorInput->m_TextureHandle);

  ezGALResourceViewHandle colorResourceViewHandle = pDevice->GetDefaultResourceView(hSceneColor);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", colorResourceViewHandle);
  renderViewContext.m_pRenderContext->BindSamplerState("SceneColorSampler", m_hSceneColorSamplerState);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);

  ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hSceneColor);
}

void ezTransparentForwardRenderPass::SetupResources(ezGALPass* pGALPass, const ezRenderViewContext& renderViewContext,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    ezGALResourceViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", depthResourceViewHandle);
  }
}

void ezTransparentForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
}

void ezTransparentForwardRenderPass::UpdateSceneColorTexture(
  const ezRenderViewContext& renderViewContext, ezGALTextureHandle hSceneColorTexture, ezGALTextureHandle hCurrentColorTexture)
{
  ezGALTextureSubresource subresource;
  subresource.m_uiMipLevel = 0;
  subresource.m_uiArraySlice = 0;

  renderViewContext.m_pRenderContext->GetCommandEncoder()->ResolveTexture(hSceneColorTexture, subresource, hCurrentColorTexture, subresource);
}

void ezTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = ezGALTextureFilterMode::Linear;
    desc.m_MagFilter = ezGALTextureFilterMode::Linear;
    desc.m_MipFilter = ezGALTextureFilterMode::Linear;
    desc.m_AddressU = ezImageAddressMode::Clamp;
    desc.m_AddressV = ezImageAddressMode::Mirror;
    desc.m_AddressW = ezImageAddressMode::Mirror;

    m_hSceneColorSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
