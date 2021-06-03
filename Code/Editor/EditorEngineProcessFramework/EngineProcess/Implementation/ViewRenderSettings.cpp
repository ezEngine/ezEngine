#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSceneViewPerspective, 1)
  EZ_ENUM_CONSTANTS(ezSceneViewPerspective::Orthogonal_Front, ezSceneViewPerspective::Orthogonal_Right, ezSceneViewPerspective::Orthogonal_Top,
    ezSceneViewPerspective::Perspective)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineViewLightSettings, 1, ezRTTIDefaultAllocator<ezEngineViewLightSettings>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("SkyBox", m_bSkyBox),
      EZ_MEMBER_PROPERTY("SkyLight", m_bSkyLight),
      EZ_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap),
      EZ_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity),
      EZ_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      EZ_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      EZ_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      EZ_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity),
      EZ_MEMBER_PROPERTY("Fog", m_bFog)
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezEngineViewConfig::ApplyPerspectiveSetting(float fov, float nearPlane, float farPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case ezSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovY, fov == 0.0f ? 70.0f : fov, nearPlane, farPlane);
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(-1, 0, 0), ezVec3(0, 0, 1));
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, -1, 0), ezVec3(0, 0, 1));
    }
    break;

    case ezSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(ezCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + ezVec3(0, 0, -1), ezVec3(1, 0, 0));
    }
    break;
  }
}

ezEngineViewLightSettings::ezEngineViewLightSettings(bool bEnable)
{
  if (!bEnable)
  {
    m_bSkyLight = false;
    m_bDirectionalLight = false;
    m_bFog = false;
  }
}

ezEngineViewLightSettings::~ezEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

bool ezEngineViewLightSettings::GetSkyBox() const
{
  return m_bSkyBox;
}

void ezEngineViewLightSettings::SetSkyBox(bool val)
{
  m_bSkyBox = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool ezEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void ezEngineViewLightSettings::SetSkyLight(bool val)
{
  m_bSkyLight = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* ezEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void ezEngineViewLightSettings::SetSkyLightCubeMap(const char* val)
{
  m_sSkyLightCubeMap = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float ezEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void ezEngineViewLightSettings::SetSkyLightIntensity(float val)
{
  m_fSkyLightIntensity = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
}

bool ezEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void ezEngineViewLightSettings::SetDirectionalLight(bool val)
{
  m_bDirectionalLight = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
}

ezAngle ezEngineViewLightSettings::GetDirectionalLightAngle() const
{
  return m_DirectionalLightAngle;
}

void ezEngineViewLightSettings::SetDirectionalLightAngle(ezAngle val)
{
  m_DirectionalLightAngle = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
}

bool ezEngineViewLightSettings::GetDirectionalLightShadows() const
{
  return m_bDirectionalLightShadows;
}

void ezEngineViewLightSettings::SetDirectionalLightShadows(bool val)
{
  m_bDirectionalLightShadows = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float ezEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void ezEngineViewLightSettings::SetDirectionalLightIntensity(float val)
{
  m_fDirectionalLightIntensity = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool ezEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void ezEngineViewLightSettings::SetFog(bool val)
{
  m_bFog = val;
  SetModifiedInternal(ezEngineViewLightSettingsEvent::Type::FogChanged);
}

bool ezEngineViewLightSettings::SetupForEngine(ezWorld* pWorld, ezUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

namespace
{
  template <typename T>
  T* SyncComponent(ezWorld* pWorld, ezGameObject* pParent, ezComponentHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetComponent(handle, pComp))
      {
        handle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(handle, pComp))
        {
          pComp->DeleteComponent();
          handle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  ezGameObject* SyncGameObject(ezWorld* pWorld, ezGameObjectHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      ezGameObject* pObj = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetObject(handle, pObj))
      {
        ezGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        handle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(handle);
      }
      return nullptr;
    }
  }
} // namespace

void ezEngineViewLightSettings::UpdateForEngine(ezWorld* pWorld)
{
  if (ezGameObject* pParent = SyncGameObject(m_pWorld, m_hSkyBoxObject, m_bSkyBox))
  {
    ezTagSet& tags = pParent->GetTags();
    tags.SetByName("SkyLight");

    if (ezSkyBoxComponent* pSkyBox = SyncComponent<ezSkyBoxComponent>(m_pWorld, pParent, m_hSkyBox, m_bSkyBox))
    {
      pSkyBox->SetCubeMapFile(m_sSkyLightCubeMap);
    }
  }

  const bool bNeedGameObject = m_bDirectionalLight | m_bSkyLight;
  if (ezGameObject* pParent = SyncGameObject(m_pWorld, m_hGameObject, bNeedGameObject))
  {
    ezQuat rot;
    rot.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), m_DirectionalLightAngle + ezAngle::Degree(90.0));
    pParent->SetLocalRotation(rot);

    if (ezDirectionalLightComponent* pDirLight = SyncComponent<ezDirectionalLightComponent>(m_pWorld, pParent, m_hDirLight, m_bDirectionalLight))
    {
      pDirLight->SetCastShadows(m_bDirectionalLightShadows);
      pDirLight->SetIntensity(m_fDirectionalLightIntensity);
    }

    if (ezSkyLightComponent* pSkyLight = SyncComponent<ezSkyLightComponent>(m_pWorld, pParent, m_hSkyLight, m_bSkyLight))
    {
      pSkyLight->SetIntensity(m_fSkyLightIntensity);
      pSkyLight->SetReflectionProbeMode(ezReflectionProbeMode::Static);
      pSkyLight->SetCubeMapFile(m_sSkyLightCubeMap);
    }

    if (ezFogComponent* pFog = SyncComponent<ezFogComponent>(m_pWorld, pParent, m_hFog, m_bFog))
    {
      pFog->SetColor(ezColor(0.02f, 0.02f, 0.02f));
      pFog->SetDensity(5.0f);
      pFog->SetHeightFalloff(0);
    }
  }
}

void ezEngineViewLightSettings::SetModifiedInternal(ezEngineViewLightSettingsEvent::Type type)
{
  SetModified();
  ezEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
