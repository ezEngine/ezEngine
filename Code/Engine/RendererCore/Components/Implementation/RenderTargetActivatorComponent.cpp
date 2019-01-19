#include <PCH.h>

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
    EZ_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new ezAssetBrowserAttribute("Render Target")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezRenderTargetActivatorComponent::ezRenderTargetActivatorComponent() = default;
ezRenderTargetActivatorComponent::~ezRenderTargetActivatorComponent() = default;

void ezRenderTargetActivatorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_hRenderTarget;
}

void ezRenderTargetActivatorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hRenderTarget;
}

ezResult ezRenderTargetActivatorComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hRenderTarget.IsValid())
  {
    bounds = ezBoundingSphere(ezVec3::ZeroVector(), 0.1f);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezRenderTargetActivatorComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  ezResourceLock<ezRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::NoFallback);

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

void ezRenderTargetActivatorComponent::SetRenderTargetFile(const char* szFile)
{
  ezRenderToTexture2DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezRenderToTexture2DResource>(szFile);
  }

  SetRenderTarget(hResource);
}

const char* ezRenderTargetActivatorComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
