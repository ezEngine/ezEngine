#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRenderTargetActivatorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("RenderTarget", GetRenderTarget, SetRenderTarget)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_Target", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezRenderTargetActivatorComponent::ezRenderTargetActivatorComponent() = default;
ezRenderTargetActivatorComponent::~ezRenderTargetActivatorComponent() = default;

void ezRenderTargetActivatorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void ezRenderTargetActivatorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

ezResult ezRenderTargetActivatorComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 0.1f);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezRenderTargetActivatorComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  ezResourceLock<ezRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::BlockTillLoaded);

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    ezRenderWorld::AddViewToRender(hView);
  }
}

void ezRenderTargetActivatorComponent::SetRenderTarget(const ezRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
