#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesUser, 1, ezRTTIDefaultAllocator<ezEditorPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new ezDefaultValueAttribute(70.0f), new ezClampValueAttribute(10.0f, 150.0f)),
    EZ_MEMBER_PROPERTY("GizmoSize", m_fGizmoSize)->AddAttributes(new ezDefaultValueAttribute(1.5f), new ezClampValueAttribute(0.2f, 5.0f)),
    EZ_MEMBER_PROPERTY("UseOldGizmos", m_bOldGizmos),
    EZ_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    EZ_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(15.0f))),
    EZ_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new ezDefaultValueAttribute(0.125f)),
    EZ_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new ezDefaultValueAttribute(0.25f)),
    EZ_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("AssetFilterCombobox", m_bAssetFilterCombobox)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new ezDefaultValueAttribute(true), new ezGroupAttribute("Engine View Light Settings")),
    EZ_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new ezDefaultValueAttribute(true), new ezClampValueAttribute(0.0f, 2.0f)),
    EZ_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new ezDefaultValueAttribute(ezStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new ezAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    EZ_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 20.0f)),
    EZ_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(30.0f)), new ezClampValueAttribute(ezAngle::Degree(-90.0f), ezAngle::Degree(90.0f))),
    EZ_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    EZ_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("Fog", m_bFog),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEditorPreferencesUser::ezEditorPreferencesUser()
  : ezPreferences(Domain::Application, "General")
{
}

ezEditorPreferencesUser::~ezEditorPreferencesUser() = default;

void ezEditorPreferencesUser::ApplyDefaultValues(ezEngineViewLightSettings& settings)
{
  settings.SetSkyBox(m_bSkyBox);
  settings.SetSkyLight(m_bSkyLight);
  settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  settings.SetDirectionalLight(m_bDirectionalLight);
  settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  settings.SetFog(m_bFog);
}

void ezEditorPreferencesUser::SetAsDefaultValues(const ezEngineViewLightSettings& settings)
{
  m_bSkyBox = settings.GetSkyBox();
  m_bSkyLight = settings.GetSkyLight();
  m_sSkyLightCubeMap = settings.GetSkyLightCubeMap();
  m_fSkyLightIntensity = settings.GetSkyLightIntensity();
  m_bDirectionalLight = settings.GetDirectionalLight();
  m_DirectionalLightAngle = settings.GetDirectionalLightAngle();
  m_bDirectionalLightShadows = settings.GetDirectionalLightShadows();
  m_fDirectionalLightIntensity = settings.GetDirectionalLightIntensity();
  m_bFog = settings.GetFog();
  TriggerPreferencesChangedEvent();
}

void ezEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;

  ezQtAddSubElementButton::s_bShowInDevelopmentFeatures = b;
}

void ezQtEditorApp::LoadEditorPreferences()
{
  EZ_PROFILE_SCOPE("Preferences");
  ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
}
