#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderer.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiBlitConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiRenderer, 1, ezRTTIDefaultAllocator<ezRmlUiRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiRenderer::ezRmlUiRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RmlUiBlit.ezShader");
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezRmlUiBlitConstants>();
}

ezRmlUiRenderer::~ezRmlUiRenderer()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

void ezRmlUiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezRmlUiRenderData>());
}

void ezRmlUiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezRmlUiRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

  pRenderContext->BindShader(m_hShader);
  pRenderContext->BindConstantBuffer("ezRmlUiBlitConstants", m_hConstantBuffer);
  pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::TriangleStrip, 2);

  const ezVec2 targetSize = ezVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
  const ezVec2 scale = ezVec2(2.0f, -2.0f).CompDiv(targetSize);
  const ezVec2 offset = ezVec2(-1.0f, 1.0f);

  for (auto it = batch.GetIterator<ezRmlUiRenderData>(); it.IsValid(); ++it)
  {
    const ezRmlUiRenderData* pRenderData = it;
    const ezGALTexture* pTexture = pDevice->GetTexture(pRenderData->m_hTexture);

    const ezVec2 targetSize = ezVec2(renderViewContext.m_pViewData->m_ViewPortRect.width, renderViewContext.m_pViewData->m_ViewPortRect.height);
    const ezVec2 textureSize = ezVec2(static_cast<float>(pTexture->GetDescription().m_uiWidth), static_cast<float>(pTexture->GetDescription().m_uiHeight));

    ezRmlUiBlitConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiBlitConstants>(m_hConstantBuffer);
    pConstants->Scale = textureSize.CompMul(scale);
    pConstants->Offset = pRenderData->m_vOffset.CompMul(scale) + offset;

    pRenderContext->BindTexture2D("BaseTexture", pDevice->GetDefaultResourceView(pRenderData->m_hTexture));

    pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Implementation_RmlUiRenderer);
