#include <RasterizerExplorer/RasterizerOverlayComponent.h>

#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>

// clang-format off

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRasterizerOverlayRenderData, 1, ezRTTIDefaultAllocator<ezRasterizerOverlayRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezRasterizerOverlayComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRasterizerOverlayRenderer, 1, ezRTTIDefaultAllocator<ezRasterizerOverlayRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

// --- Component ---

ezRasterizerOverlayComponent::ezRasterizerOverlayComponent() = default;
ezRasterizerOverlayComponent::~ezRasterizerOverlayComponent() = default;

ezResult ezRasterizerOverlayComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezRasterizerOverlayComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (m_hTexture.IsInvalidated())
    return;

  auto* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezRasterizerOverlayRenderData>(GetOwner());
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_uiSortingKey = 0xFFFFFFFF; // render last

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, ezRenderData::Caching::Never);
}

// --- Renderer ---

ezRasterizerOverlayRenderer::ezRasterizerOverlayRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RasterizerOverlay.ezShader");
}

ezRasterizerOverlayRenderer::~ezRasterizerOverlayRenderer() = default;

void ezRasterizerOverlayRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezRasterizerOverlayRenderData>());
}

void ezRasterizerOverlayRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  const ezRasterizerOverlayRenderData* pRenderData = batch.GetFirstData<ezRasterizerOverlayRenderData>();
  if (pRenderData == nullptr)
    return;

  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  pContext->BindShader(m_hShader);

  ezBindGroupBuilder& bindGroup = pContext->GetBindGroup();
  bindGroup.BindTexture("OverlayTexture", pRenderData->m_hTexture);

  // Fullscreen triangle: 1 triangle = 3 vertices covers the whole screen
  pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
  pContext->DrawMeshBuffer().IgnoreResult();
}
