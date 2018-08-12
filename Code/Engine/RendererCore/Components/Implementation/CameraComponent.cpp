#include <PCH.h>

#include <Core/ResourceManager/ResourceBase.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>


ezCameraComponentManager::ezCameraComponentManager(ezWorld* pWorld)
    : ezComponentManager<ezCameraComponent, ezBlockStorageType::Compact>(pWorld)
{
}

ezCameraComponentManager::~ezCameraComponentManager() = default;

void ezCameraComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezCameraComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);

  ezRenderWorld::s_ViewCreatedEvent.AddEventHandler(ezMakeDelegate(&ezCameraComponentManager::OnViewCreated, this));
}


void ezCameraComponentManager::Deinitialize()
{
  ezRenderWorld::s_ViewCreatedEvent.RemoveEventHandler(ezMakeDelegate(&ezCameraComponentManager::OnViewCreated, this));
}

void ezCameraComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto hCameraComponent : m_modifiedCameras)
  {
    ezCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    ezCameraUsageHint::Enum usageHint = pCameraComponent->GetUsageHint();

    if (ezView* pView = ezRenderWorld::GetViewByUsageHint(usageHint))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_modifiedCameras.Clear();

  for (auto hCameraComponent : m_RenderTargetCameras)
  {
    ezCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    pCameraComponent->UpdateRenderTargetCamera();
  }
}

const ezCameraComponent* ezCameraComponentManager::GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

ezCameraComponent* ezCameraComponentManager::GetCameraByUsageHint(ezCameraUsageHint::Enum usageHint)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

void ezCameraComponentManager::AddRenderTargetCamera(ezCameraComponent* pComponent)
{
  m_RenderTargetCameras.PushBack(pComponent->GetHandle());
}

void ezCameraComponentManager::RemoveRenderTargetCamera(ezCameraComponent* pComponent)
{
  m_RenderTargetCameras.RemoveSwap(pComponent->GetHandle());
}

void ezCameraComponentManager::OnViewCreated(ezView* pView)
{
  // Mark all cameras as modified so the new view gets the proper settings
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->MarkAsModified(this);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezCameraComponent, 7, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("UsageHint", ezCameraUsageHint, GetUsageHint, SetUsageHint),
    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezCameraMode, GetCameraMode, SetCameraMode),
    EZ_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new ezAssetBrowserAttribute("Render Target")),
    EZ_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new ezClampValueAttribute(ezVec2(0.0f), ezVec2(0.9f))),
    EZ_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new ezDefaultValueAttribute(ezVec2(1.0f)), new ezClampValueAttribute(ezVec2(0.1f), ezVec2(1.0f))),
    EZ_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.01f, 4.0f)),
    EZ_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new ezDefaultValueAttribute(1000.0f), new ezClampValueAttribute(5.0, 10000.0f)),
    EZ_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new ezDefaultValueAttribute(60.0f), new ezClampValueAttribute(1.0f, 170.0f)),
    EZ_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.01f, 10000.0f)),
    EZ_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_ACCESSOR_PROPERTY("RenderPipeline", GetRenderPipelineFile, SetRenderPipelineFile)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline")),
    EZ_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f, 32.0f), new ezSuffixAttribute(" f-stop(s)")),
    EZ_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f/100000.0f, 600.0f), new ezSuffixAttribute(" s")),
    EZ_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(50.0f, 64000.0f)),
    EZ_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new ezClampValueAttribute(-32.0f, 32.0f)),
    EZ_MEMBER_PROPERTY("ShowStats", m_bShowStats),
    /*EZ_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Final Exposure", GetExposure),*/
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::DarkSlateBlue),
    new ezCameraVisualizerAttribute("Mode", "FOV", "Dimensions", "NearPlane", "FarPlane"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCameraComponent::ezCameraComponent()
{
  m_fNearPlane = 0.25f;
  m_fFarPlane = 1000.0f;
  m_fPerspectiveFieldOfView = 60.0f;
  m_fOrthoDimension = 10.0f;

  m_fAperture = 1.0f;
  m_fShutterTime = 1.0f;
  m_fISO = 100.0f;
  m_fExposureCompensation = 0.0f;

  m_bIsModified = false;
  m_bShowStats = false;
}

ezCameraComponent::~ezCameraComponent() = default;

void ezCameraComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;

  // Version 2
  s << m_hRenderPipeline;

  // Version 3
  s << m_fAperture;
  s << m_fShutterTime;
  s << m_fISO;
  s << m_fExposureCompensation;

  // Version 4
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  // Version 6
  s << m_hRenderTarget;

  // Version 7
  s << m_RenderTargetRectOffset;
  s << m_RenderTargetRectSize;
}

void ezCameraComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  ezCameraUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > ezCameraUsageHint::MainView)
    usage = ezCameraUsageHint::None;
  m_UsageHint.SetValue(usage);

  ezCameraMode::StorageType cam;
  s >> cam;
  m_Mode.SetValue(cam);

  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;

  if (uiVersion >= 2)
  {
    s >> m_hRenderPipeline;
  }

  if (uiVersion >= 3)
  {
    s >> m_fAperture;
    s >> m_fShutterTime;
    s >> m_fISO;
    s >> m_fExposureCompensation;
  }

  if (uiVersion >= 4)
  {
    m_IncludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
  }

  if (uiVersion >= 6)
  {
    s >> m_hRenderTarget;
  }

  if (uiVersion >= 7)
  {
    s >> m_RenderTargetRectOffset;
    s >> m_RenderTargetRectSize;
  }

  MarkAsModified();
}

void ezCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == ezCameraMode::PerspectiveFixedFovX || m_Mode == ezCameraMode::PerspectiveFixedFovY)
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  else
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);

  m_RenderTargetCamera.LookAt(GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(),
                              GetOwner()->GetGlobalDirUp());
}

void ezCameraComponent::SetUsageHint(ezEnum<ezCameraUsageHint> val)
{
  if (val == m_UsageHint)
    return;

  DeactivateRenderToTexture();

  m_UsageHint = val;

  ActivateRenderToTexture();

  MarkAsModified();
}

void ezCameraComponent::SetRenderTargetFile(const char* szFile)
{
  DeactivateRenderToTexture();

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hRenderTarget = ezResourceManager::LoadResource<ezTexture2DResource>(szFile);
  }
  else
  {
    m_hRenderTarget.Invalidate();
  }

  ActivateRenderToTexture();

  MarkAsModified();
}

const char* ezCameraComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}

void ezCameraComponent::SetRenderTargetRectOffset(ezVec2 value)
{
  DeactivateRenderToTexture();

  m_RenderTargetRectOffset.x = ezMath::Clamp(value.x, 0.0f, 0.9f);
  m_RenderTargetRectOffset.y = ezMath::Clamp(value.y, 0.0f, 0.9f);

  ActivateRenderToTexture();
}

void ezCameraComponent::SetRenderTargetRectSize(ezVec2 value)
{
  DeactivateRenderToTexture();

  m_RenderTargetRectSize.x = ezMath::Clamp(value.x, 0.1f, 1.0f);
  m_RenderTargetRectSize.y = ezMath::Clamp(value.y, 0.1f, 1.0f);

  ActivateRenderToTexture();
}

void ezCameraComponent::SetCameraMode(ezEnum<ezCameraMode> val)
{
  if (val == m_Mode)
    return;
  m_Mode = val;

  MarkAsModified();
}


void ezCameraComponent::SetNearPlane(float val)
{
  if (val == m_fNearPlane)
    return;
  m_fNearPlane = val;

  MarkAsModified();
}


void ezCameraComponent::SetFarPlane(float val)
{
  if (val == m_fFarPlane)
    return;
  m_fFarPlane = val;

  MarkAsModified();
}


void ezCameraComponent::SetFieldOfView(float val)
{
  if (val == m_fPerspectiveFieldOfView)
    return;
  m_fPerspectiveFieldOfView = val;

  MarkAsModified();
}


void ezCameraComponent::SetOrthoDimension(float val)
{
  if (val == m_fOrthoDimension)
    return;
  m_fOrthoDimension = val;

  MarkAsModified();
}

void ezCameraComponent::SetRenderPipeline(ezRenderPipelineResourceHandle hRenderPipeline)
{
  if (hRenderPipeline == m_hRenderPipeline)
    return;

  DeactivateRenderToTexture();

  m_hRenderPipeline = hRenderPipeline;

  ActivateRenderToTexture();

  MarkAsModified();
}

const char* ezCameraComponent::GetRenderPipelineFile() const
{
  if (!m_hRenderPipeline.IsValid())
    return "";

  return m_hRenderPipeline.GetResourceID();
}

void ezCameraComponent::SetRenderPipelineFile(const char* szFile)
{
  ezRenderPipelineResourceHandle hRenderPipeline;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hRenderPipeline = ezResourceManager::LoadResource<ezRenderPipelineResource>(szFile);
  }

  SetRenderPipeline(hRenderPipeline);
}

void ezCameraComponent::SetAperture(float fAperture)
{
  if (m_fAperture == fAperture)
    return;
  m_fAperture = fAperture;

  MarkAsModified();
}

void ezCameraComponent::SetShutterTime(float fShutterTime)
{
  if (m_fShutterTime == fShutterTime)
    return;
  m_fShutterTime = fShutterTime;

  MarkAsModified();
}

void ezCameraComponent::SetISO(float fISO)
{
  if (m_fISO == fISO)
    return;
  m_fISO = fISO;

  MarkAsModified();
}

void ezCameraComponent::SetExposureCompensation(float fEC)
{
  if (m_fExposureCompensation == fEC)
    return;
  m_fExposureCompensation = fEC;

  MarkAsModified();
}

float ezCameraComponent::GetEV100() const
{
  // From: course_notes_moving_frostbite_to_pbr.pdf
  // EV number is defined as:
  // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
  // This gives
  // EV_s = log2 (N^2 / t)
  // EV_100 + log2 (S /100) = log2 (N^2 / t)
  // EV_100 = log2 (N^2 / t) - log2 (S /100)
  // EV_100 = log2 (N^2 / t . 100 / S)
  return ezMath::Log2((m_fAperture * m_fAperture) / m_fShutterTime * 100.0f / m_fISO) - m_fExposureCompensation;
}

float ezCameraComponent::GetExposure() const
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  float maxLuminance = 1.2f * ezMath::Pow2(GetEV100());
  return 1.0f / maxLuminance;
}

void ezCameraComponent::ApplySettingsToView(ezView* pView) const
{
  if (m_UsageHint == ezCameraUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == ezCameraMode::OrthoFixedWidth || m_Mode == ezCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  ezCamera* pCamera = pView->GetCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, ezMath::Max(m_fNearPlane + 0.00001f, m_fFarPlane));
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  if (m_bShowStats)
  {
    // draw stats
    {
      const char* szName = GetOwner()->GetName();

      ezStringBuilder sb;
      sb.Format("Camera '{0}': EV100: {1}, Exposure: {2}", ezStringUtils::IsNullOrEmpty(szName) ? pView->GetName() : szName, GetEV100(),
                GetExposure());
      ezDebugRenderer::DrawText(GetWorld(), sb, ezVec2I32(20, 20), ezColor::LimeGreen);
    }

    // draw frustum
    {
      const ezGameObject* pOwner = GetOwner();
      ezVec3 vPosition = pOwner->GetGlobalPosition();
      ezVec3 vForward = pOwner->GetGlobalDirForwards();
      ezVec3 vUp = pOwner->GetGlobalDirUp();

      ezMat4 viewMatrix;
      viewMatrix.SetLookAtMatrix(vPosition, vPosition + vForward, vUp);

      ezMat4 projectionMatrix = pView->GetProjectionMatrix(ezCameraEye::Left); // todo: Stereo support
      ezMat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

      ezFrustum frustum;
      frustum.SetFrustum(vPosition, viewProjectionMatrix, 10.0f);

      ezDebugRenderer::DrawLineFrustum(GetWorld(), frustum, ezColor::LimeGreen);
    }
  }

  if (m_hRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hRenderPipeline);
  }
}

void ezCameraComponent::ResourceChangeEventHandler(const ezResourceEvent& e)
{
  switch (e.m_EventType)
  {
    case ezResourceEventType::ResourceExists:
    case ezResourceEventType::ResourceCreated:
    case ezResourceEventType::ResourceInPreloadQueue:
    case ezResourceEventType::ResourceOutOfPreloadQueue:
    case ezResourceEventType::ResourcePriorityChanged:
    case ezResourceEventType::ResourceDueDateChanged:
      return;

    case ezResourceEventType::ResourceDeleted:
    case ezResourceEventType::ResourceContentUnloading:
    case ezResourceEventType::ResourceContentUpdated:
      // triggers a recreation of the view
      ezRenderWorld::DeleteView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}

void ezCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetWorld()->GetComponentManager<ezCameraComponentManager>()->m_modifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}


void ezCameraComponent::MarkAsModified(ezCameraComponentManager* pCameraManager)
{
  if (!m_bIsModified)
  {
    pCameraManager->m_modifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}

void ezCameraComponent::ActivateRenderToTexture()
{
  if (m_UsageHint != ezCameraUsageHint::RenderTarget)
    return;

  if (m_bRenderTargetInitialized || !m_hRenderTarget.IsValid() || !m_hRenderPipeline.IsValid() || !IsActiveAndInitialized())
    return;

  ezResourceLock<ezTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::NoFallback);

  if (pRenderTarget->IsMissingResource())
  {
    return;
  }

  m_bRenderTargetInitialized = true;

  EZ_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  ezStringBuilder name;
  name.Format("Camera RT: {0}", GetOwner()->GetName());

  ezView* pRenderTargetView = nullptr;
  m_hRenderTargetView = ezRenderWorld::CreateView(name, pRenderTargetView);

  pRenderTargetView->SetRenderPipelineResource(m_hRenderPipeline);

  pRenderTargetView->SetWorld(GetWorld());
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  pRenderTarget->m_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezCameraComponent::ResourceChangeEventHandler, this));

  ezGALRenderTagetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, pRenderTarget->GetRenderTargetView());
  pRenderTargetView->SetRenderTargetSetup(renderTargetSetup);

  const float maxSizeX = 1.0f - m_RenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_RenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width = resX * ezMath::Min(maxSizeX, m_RenderTargetRectSize.x);
  const float height = resY * ezMath::Min(maxSizeY, m_RenderTargetRectSize.y);

  const float offsetX = m_RenderTargetRectOffset.x * resX;
  const float offsetY = m_RenderTargetRectOffset.y * resY;

  pRenderTargetView->SetViewport(ezRectFloat(offsetX, offsetY, width, height));

  pRenderTarget->AddRenderView(m_hRenderTargetView);

  GetWorld()->GetComponentManager<ezCameraComponentManager>()->AddRenderTargetCamera(this);
}

void ezCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;

  if (m_hRenderTarget.IsValid())
  {
    ezResourceLock<ezTexture2DResource> pRenderTarget(m_hRenderTarget, ezResourceAcquireMode::NoFallback);
    pRenderTarget->RemoveRenderView(m_hRenderTargetView);

    pRenderTarget->m_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezCameraComponent::ResourceChangeEventHandler, this));
  }

  if (!m_hRenderTargetView.IsInvalidated())
  {
    ezRenderWorld::DeleteView(m_hRenderTargetView);
    m_hRenderTargetView.Invalidate();
  }

  GetWorld()->GetComponentManager<ezCameraComponentManager>()->RemoveRenderTargetCamera(this);
}

void ezCameraComponent::OnActivated()
{
  SUPER::OnActivated();

  ActivateRenderToTexture();
}

void ezCameraComponent::OnDeactivated()
{
  DeactivateRenderToTexture();

  SUPER::OnDeactivated();
}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezCameraComponentPatch_4_5 : public ezGraphPatch
{
public:
  ezCameraComponentPatch_4_5()
      : ezGraphPatch("ezCameraComponent", 5)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Usage Hint", "UsageHint");
    pNode->RenameProperty("Near Plane", "NearPlane");
    pNode->RenameProperty("Far Plane", "FarPlane");
    pNode->RenameProperty("Include Tags", "IncludeTags");
    pNode->RenameProperty("Exclude Tags", "ExcludeTags");
    pNode->RenameProperty("Render Pipeline", "RenderPipeline");
    pNode->RenameProperty("Shutter Time", "ShutterTime");
    pNode->RenameProperty("Exposure Compensation", "ExposureCompensation");
  }
};

ezCameraComponentPatch_4_5 g_ezCameraComponentPatch_4_5;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_CameraComponent);
