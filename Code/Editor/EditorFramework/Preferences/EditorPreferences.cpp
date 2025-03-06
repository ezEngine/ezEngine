#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorPreferencesUser, 1, ezRTTIDefaultAllocator<ezEditorPreferencesUser>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("MaxAssetProcessors", m_uiMaxAssetProcessors)->AddAttributes(new ezDefaultValueAttribute(8), new ezClampValueAttribute(1, 8)),
    EZ_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new ezDefaultValueAttribute(70.0f), new ezClampValueAttribute(10.0f, 150.0f)),
    EZ_MEMBER_PROPERTY("MaxFramerate", m_uiMaxFramerate)->AddAttributes(new ezDefaultValueAttribute(60)),
    EZ_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new ezDefaultValueAttribute(1.5f), new ezClampValueAttribute(0.2f, 5.0f)),
    EZ_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    EZ_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(15.0f)), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new ezDefaultValueAttribute(0.125f), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("CustomPrecompiledToolsFolder", m_sCustomPrecompiledToolsFolder),
    EZ_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("CombinedEditorAndEngineLogs", m_bCombinedEditorAndEngineLogs)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),
    EZ_MEMBER_PROPERTY("AssetBrowserShowItemsInSubFolders", m_bAssetBrowserShowItemsInSubFolders)->AddAttributes(new ezDefaultValueAttribute(true), new ezHiddenAttribute()),

    // START GROUP Engine View Light Settings
    EZ_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new ezDefaultValueAttribute(true), new ezGroupAttribute("Engine View Light Settings")),
    EZ_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new ezDefaultValueAttribute(true), new ezClampValueAttribute(0.0f, 2.0f)),
    EZ_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new ezDefaultValueAttribute(ezStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new ezAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    EZ_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 20.0f)),
    EZ_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(70.0f)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(360.0f))),
    EZ_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    EZ_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("Fog", m_bFog),
    EZ_ARRAY_MEMBER_PROPERTY("RecentTypes", m_RecentlyCreatedTypes)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezEditorPreferencesUser::ezEditorPreferencesUser()
  : ezPreferences(Domain::Application, "General")
{
  ezQtTypeMenu::s_pRecentList = &m_RecentlyCreatedTypes;
}

ezEditorPreferencesUser::~ezEditorPreferencesUser()
{
  ezQtTypeMenu::s_pRecentList = nullptr;
}

void ezEditorPreferencesUser::ApplyDefaultValues(ezEngineViewLightSettings& ref_settings)
{
  ref_settings.SetSkyBox(m_bSkyBox);
  ref_settings.SetSkyLight(m_bSkyLight);
  ref_settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  ref_settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  ref_settings.SetDirectionalLight(m_bDirectionalLight);
  ref_settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  ref_settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  ref_settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  ref_settings.SetFog(m_bFog);
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

  ezQtTypeMenu::s_bShowInDevelopmentFeatures = b;
}

void ezEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;

  ezTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
}

void ezEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
  SyncGlobalSettings();
}


void ezEditorPreferencesUser::SetMaxFramerate(ezUInt16 uiFPS)
{
  if (m_uiMaxFramerate == uiFPS)
    return;

  m_uiMaxFramerate = uiFPS;
}

void ezEditorPreferencesUser::SyncGlobalSettings()
{
  ezGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = m_fGizmoSize;

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtEditorApp::LoadEditorPreferences()
{
  EZ_PROFILE_SCOPE("Preferences");
  ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
}
