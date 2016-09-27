#include <RendererCore/PCH.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>


ezCameraComponentManager::ezCameraComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezCameraComponent, true>(pWorld)
{

}

ezCameraComponentManager::~ezCameraComponentManager()
{

}

void ezCameraComponentManager::Initialize()
{
  auto desc = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(ezCameraComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void ezCameraComponentManager::Update(ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  for (auto hCameraComponent : m_modifiedCameras)
  {
    ezCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    ezCameraComponentUsageHint::Enum usageHint = pCameraComponent->GetUsageHint();

    for (auto pView : ezRenderLoop::GetAllViews())
    {
      if (pView->GetCameraUsageHint() == usageHint)
      {
        pCameraComponent->ApplySettingsToView(pView);
      }
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_modifiedCameras.Clear();
}

const ezCameraComponent* ezCameraComponentManager::GetCameraByUsageHint(ezCameraComponentUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezCameraComponent, 4)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("Usage Hint", ezCameraComponentUsageHint, GetUsageHint, SetUsageHint),
    EZ_ENUM_ACCESSOR_PROPERTY("Mode", ezCameraMode, GetCameraMode, SetCameraMode),
    EZ_ACCESSOR_PROPERTY("Near Plane", GetNearPlane, SetNearPlane)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 1000000.0f)),
    EZ_ACCESSOR_PROPERTY("Far Plane", GetFarPlane, SetFarPlane)->AddAttributes(new ezDefaultValueAttribute(1000.0f), new ezClampValueAttribute(0.0f, 1000000.0f)),
    EZ_ACCESSOR_PROPERTY("FOV (perspective)", GetFieldOfView, SetFieldOfView)->AddAttributes(new ezDefaultValueAttribute(60.0f), new ezClampValueAttribute(1.0f, 179.0f)),
    EZ_ACCESSOR_PROPERTY("Dimensions (ortho)", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, 1000000.0f)),
    EZ_SET_MEMBER_PROPERTY("Include Tags", m_IncludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_SET_MEMBER_PROPERTY("Exclude Tags", m_ExcludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_ACCESSOR_PROPERTY("Render Pipeline", GetRenderPipelineFile, SetRenderPipelineFile)->AddAttributes(new ezAssetBrowserAttribute("RenderPipeline")),
    EZ_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f, 32.0f), new ezSuffixAttribute(" f-stop(s)")),
    EZ_ACCESSOR_PROPERTY("Shutter Time", GetShutterTime, SetShutterTime)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f/100000.0f, 600.0f), new ezSuffixAttribute(" s")),
    EZ_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(50.0f, 64000.0f)),
    EZ_ACCESSOR_PROPERTY("Exposure Compensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new ezClampValueAttribute(-32.0f, 32.0f)),
    /*EZ_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Final Exposure", GetExposure),*/
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::DarkSlateBlue),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

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
}

ezCameraComponent::~ezCameraComponent()
{

}

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
}

void ezCameraComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  ezCameraComponentUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > ezCameraComponentUsageHint::MainView)
    usage = ezCameraComponentUsageHint::None;
  m_UsageHint.SetValue(usage);

  ezCameraMode::StorageType cam;
  s >> cam; m_Mode.SetValue(cam);

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

  MarkAsModified();
}

void ezCameraComponent::SetUsageHint(ezEnum<ezCameraComponentUsageHint> val)
{
  if (val == m_UsageHint)
    return;
  m_UsageHint = val;
  
  MarkAsModified();
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
  m_hRenderPipeline = hRenderPipeline;
  
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
  if (m_UsageHint == ezCameraComponentUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == ezCameraMode::OrthoFixedWidth || m_Mode == ezCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  ezCamera* pCamera = pView->GetLogicCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, m_fFarPlane);
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  //ezLog::Info("EV100: %f, Exposure: %f", GetEV100(), GetExposure());

  if (m_hRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hRenderPipeline);
  }
}

void ezCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetManager()->m_modifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}
