#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPickingRenderPass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPickingRenderPass::ezPickingRenderPass() : ezRenderPipelinePass( "EditorPickingRenderPass" )
{
  m_pSceneContext = nullptr;
  m_bEnable = true;
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
}

ezPickingRenderPass::~ezPickingRenderPass()
{
  DestroyTarget();
}

ezGALTextureHandle ezPickingRenderPass::GetPickingIdRT() const
{
  return m_hPickingIdRT;
}

ezGALTextureHandle ezPickingRenderPass::GetPickingDepthRT() const
{
  return m_hPickingDepthRT;
}

bool ezPickingRenderPass::GetRenderTargetDescriptions(const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  return true;
}

void ezPickingRenderPass::SetRenderTargets(const ezArrayPtr<ezRenderPipelinePassConnection*const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection*const> outputs)
{
  DestroyTarget();
  CreateTarget();
}

void ezPickingRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  if (!m_bEnable)
    return;

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::WireframeColor:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_COLOR");
    break;
  case ezViewRenderMode::WireframeMonochrome:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_MONOCHROME");
    break;
  default:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");
    break;
  }

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = 128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height;
  ezRenderContext::GetDefaultInstance()->SetMaterialParameter("GizmoScale", fGizmoScale);


  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  pGALContext->SetRenderTargetSetup(m_RenderTargetSetup);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f));

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "1");

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);

  m_pSceneContext->RenderShapeIcons(renderViewContext.m_pRenderContext);

  Event e;
  e.m_Type = Event::Type::AfterOpaque;
  m_Events.Broadcast(e);

  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "0");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");

  e.m_Type = Event::Type::EndOfFrame;
  m_Events.Broadcast(e);
}

void ezPickingRenderPass::CreateTarget()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto viewport = GetPipeline()->GetView()->GetViewport();

  // Create render target for picking
  ezGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView = false;
  tcd.m_bAllowUAV = false;
  tcd.m_bCreateRenderTarget = true;
  tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type = ezGALTextureType::Texture2D;
  tcd.m_uiWidth = (ezUInt32)viewport.width;
  tcd.m_uiHeight = (ezUInt32)viewport.height;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = ezGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = true;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);

  ezGALRenderTargetViewCreationDescription rtvd;
  rtvd.m_hTexture = m_hPickingIdRT;
  rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;
  m_hPickingIdRTV = pDevice->CreateRenderTargetView(rtvd);

  rtvd.m_hTexture = m_hPickingDepthRT;
  rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;
  m_hPickingDepthDSV = pDevice->CreateRenderTargetView(rtvd);

  m_RenderTargetSetup.SetRenderTarget(0, m_hPickingIdRTV)
    .SetDepthStencilTarget(m_hPickingDepthDSV);
}

void ezPickingRenderPass::DestroyTarget()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_RenderTargetSetup.DestroyAllAttachedViews();
  if (!m_hPickingIdRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingIdRT);
    m_hPickingIdRT.Invalidate();
  }

  if (!m_hPickingDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingDepthRT);
    m_hPickingDepthRT.Invalidate();
  }
}
