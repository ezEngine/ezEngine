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
    EZ_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Render Target")),
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

  s << m_hTexture;
}

void ezRenderTargetActivatorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hTexture;
}

ezResult ezRenderTargetActivatorComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hTexture.IsValid())
  {
    bounds.ExpandToInclude(ezBoundingBoxSphere(ezVec3::ZeroVector(), ezVec3(0.05f), 0.1f));
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

  if (!m_hTexture.IsValid())
    return;

  ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::NoFallback);

  for (auto hView : pTexture->GetAllRenderViews())
  {
    // ezLog::Debug("Adding camera view");
    ezRenderWorld::AddViewToRender(hView);
  }
}

void ezRenderTargetActivatorComponent::SetTexture(const ezTexture2DResourceHandle& hResource)
{
  m_hTexture = hResource;

  TriggerLocalBoundsUpdate();
}

void ezRenderTargetActivatorComponent::SetTextureFile(const char* szFile)
{
  ezTexture2DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezTexture2DResource>(szFile);
  }

  SetTexture(hResource);
}

const char* ezRenderTargetActivatorComponent::GetTextureFile() const
{
  if (!m_hTexture.IsValid())
    return "";

  return m_hTexture.GetResourceID();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
