#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewLightButtonAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewLightSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezViewLightActions::s_hLightMenu;
ezActionDescriptorHandle ezViewLightActions::s_hSkyBox;
ezActionDescriptorHandle ezViewLightActions::s_hSkyLight;
ezActionDescriptorHandle ezViewLightActions::s_hSkyLightCubeMap;
ezActionDescriptorHandle ezViewLightActions::s_hSkyLightIntensity;
ezActionDescriptorHandle ezViewLightActions::s_hDirLight;
ezActionDescriptorHandle ezViewLightActions::s_hDirLightAngle;
ezActionDescriptorHandle ezViewLightActions::s_hDirLightShadows;
ezActionDescriptorHandle ezViewLightActions::s_hDirLightIntensity;
ezActionDescriptorHandle ezViewLightActions::s_hFog;
ezActionDescriptorHandle ezViewLightActions::s_hSetAsDefault;

void ezViewLightActions::RegisterActions()
{
  s_hLightMenu = EZ_REGISTER_MENU_WITH_ICON("View.LightMenu", ":/EditorFramework/Icons/ViewLightMenu.svg");
  s_hSkyBox = EZ_REGISTER_ACTION_1(
    "View.SkyBox", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::SkyBoxChanged);
  s_hSkyLight = EZ_REGISTER_ACTION_1(
    "View.SkyLight", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::SkyLightChanged);
  s_hSkyLightCubeMap = EZ_REGISTER_ACTION_1(
    "View.SkyLightCubeMap", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
  s_hSkyLightIntensity = EZ_REGISTER_ACTION_1(
    "View.SkyLightIntensity", ezActionScope::Document, "View", "", ezViewLightSliderAction, ezEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);

  s_hDirLight = EZ_REGISTER_ACTION_1(
    "View.DirectionalLight", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
  s_hDirLightAngle = EZ_REGISTER_ACTION_1(
    "View.DirLightAngle", ezActionScope::Document, "View", "", ezViewLightSliderAction, ezEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
  s_hDirLightShadows = EZ_REGISTER_ACTION_1(
    "View.DirectionalLightShadows", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
  s_hDirLightIntensity = EZ_REGISTER_ACTION_1(
    "View.DirLightIntensity", ezActionScope::Document, "View", "", ezViewLightSliderAction, ezEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
  s_hFog = EZ_REGISTER_ACTION_1(
    "View.Fog", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::FogChanged);
  s_hSetAsDefault = EZ_REGISTER_ACTION_1(
    "View.SetAsDefault", ezActionScope::Document, "View", "", ezViewLightButtonAction, ezEngineViewLightSettingsEvent::Type::DefaultValuesChanged);
}

void ezViewLightActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hLightMenu);
  ezActionManager::UnregisterAction(s_hSkyBox);
  ezActionManager::UnregisterAction(s_hSkyLight);
  ezActionManager::UnregisterAction(s_hSkyLightCubeMap);
  ezActionManager::UnregisterAction(s_hSkyLightIntensity);
  ezActionManager::UnregisterAction(s_hDirLight);
  ezActionManager::UnregisterAction(s_hDirLightAngle);
  ezActionManager::UnregisterAction(s_hDirLightShadows);
  ezActionManager::UnregisterAction(s_hDirLightIntensity);
  ezActionManager::UnregisterAction(s_hFog);
  ezActionManager::UnregisterAction(s_hSetAsDefault);
}

void ezViewLightActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLightMenu, "", 2.5f);
  const ezStringView sSubPath = "View.LightMenu";
  pMap->MapAction(s_hSkyBox, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLight, sSubPath, 1.0f);
  pMap->MapAction(s_hSkyLightCubeMap, sSubPath, 2.0f);
  pMap->MapAction(s_hSkyLightIntensity, sSubPath, 3.0f);
  pMap->MapAction(s_hDirLight, sSubPath, 4.0f);
  pMap->MapAction(s_hDirLightAngle, sSubPath, 5.0f);
  pMap->MapAction(s_hDirLightShadows, sSubPath, 6.0f);
  pMap->MapAction(s_hDirLightIntensity, sSubPath, 7.0f);
  pMap->MapAction(s_hFog, sSubPath, 8.0f);
  pMap->MapAction(s_hSetAsDefault, sSubPath, 9.0f);
}

//////////////////////////////////////////////////////////////////////////

ezViewLightButtonAction::ezViewLightButtonAction(const ezActionContext& context, const char* szName, ezEngineViewLightSettingsEvent::Type button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<ezEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(ezEngineViewLightSettings::GetStaticRTTI()));
  EZ_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a ezEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(ezMakeDelegate(&ezViewLightButtonAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyBoxChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/ezSkyBoxComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/ezSkyLightComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
      SetIconPath(":/TypeIcons/ezSkyLightComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/ezDirectionalLightComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/ezDirectionalLightComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::FogChanged:
      SetCheckable(true);
      SetIconPath(":/TypeIcons/ezFogComponent.svg");
      break;
    case ezEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
      SetCheckable(false);
      SetIconPath(":/EditorFramework/Icons/ViewLightMenu.svg");
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

ezViewLightButtonAction::~ezViewLightButtonAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void ezViewLightButtonAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      m_pSettings->SetSkyBox(value.ConvertTo<bool>());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      m_pSettings->SetSkyLight(value.ConvertTo<bool>());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
      ezStringBuilder sFile = m_pSettings->GetSkyLightCubeMap();
      ezUuid assetGuid = ezConversionUtils::ConvertStringToUuid(sFile);

      ezQtAssetBrowserDlg dlg(pView, assetGuid, "CompatibleAsset_Texture_Cube");
      if (dlg.exec() == 0)
        return;

      assetGuid = dlg.GetSelectedAssetGuid();
      if (assetGuid.IsValid())
        ezConversionUtils::ToString(assetGuid, sFile);

      if (sFile.IsEmpty())
      {
        sFile = dlg.GetSelectedAssetPathRelative();

        if (sFile.IsEmpty())
        {
          sFile = dlg.GetSelectedAssetPathAbsolute();

          ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
        }
      }

      if (sFile.IsEmpty())
        return;

      m_pSettings->SetSkyLightCubeMap(sFile);
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      m_pSettings->SetDirectionalLight(value.ConvertTo<bool>());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      m_pSettings->SetDirectionalLightShadows(value.ConvertTo<bool>());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::FogChanged:
    {
      m_pSettings->SetFog(value.ConvertTo<bool>());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DefaultValuesChanged:
    {
      if (ezQtUiServices::MessageBoxQuestion("Do you want to make the current light settings the global default?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
        pPreferences->SetAsDefaultValues(*m_pSettings);
      }
    }
    break;
    default:
      break;
  }
}

void ezViewLightButtonAction::LightSettingsEventHandler(const ezEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void ezViewLightButtonAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyBoxChanged:
    {
      SetChecked(m_pSettings->GetSkyBox());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightChanged:
    {
      SetChecked(m_pSettings->GetSkyLight());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged:
    {
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLight());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged:
    {
      SetChecked(m_pSettings->GetDirectionalLightShadows());
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::FogChanged:
    {
      SetChecked(m_pSettings->GetFog());
    }
    break;
    default:
      break;
  }
}
//////////////////////////////////////////////////////////////////////////

ezViewLightSliderAction::ezViewLightSliderAction(const ezActionContext& context, const char* szName, ezEngineViewLightSettingsEvent::Type button)
  : ezSliderAction(context, szName)
{
  m_ButtonType = button;
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  m_pSettings = static_cast<ezEngineViewLightSettings*>(pView->GetDocumentWindow()->GetDocument()->FindSyncObject(ezEngineViewLightSettings::GetStaticRTTI()));
  EZ_ASSERT_DEV(m_pSettings != nullptr, "The asset document does not have a ezEngineViewLightSettings sync object.");
  m_SettingsID = m_pSettings->m_EngineViewLightSettingsEvents.AddEventHandler(ezMakeDelegate(&ezViewLightSliderAction::LightSettingsEventHandler, this));

  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
      SetIconPath(":/TypeIcons/ezSkyLightComponent.svg");
      SetRange(0, 20);
      break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
      SetIconPath(":/TypeIcons/ezDirectionalLightComponent.svg");
      SetRange(0, 360);
      break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
      SetIconPath(":/TypeIcons/ezDirectionalLightComponent.svg");
      SetRange(0, 200);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  UpdateAction();
}

ezViewLightSliderAction::~ezViewLightSliderAction()
{
  m_pSettings->m_EngineViewLightSettingsEvents.RemoveEventHandler(m_SettingsID);
}

void ezViewLightSliderAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      m_pSettings->SetSkyLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      m_pSettings->SetDirectionalLightAngle(ezAngle::MakeFromDegree(value.ConvertTo<float>()));
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      m_pSettings->SetDirectionalLightIntensity(value.ConvertTo<float>() / 10.0f);
    }
    break;
    default:
      break;
  }
}

void ezViewLightSliderAction::LightSettingsEventHandler(const ezEngineViewLightSettingsEvent& e)
{
  if (m_ButtonType == e.m_Type)
  {
    UpdateAction();
  }
}

void ezViewLightSliderAction::UpdateAction()
{
  switch (m_ButtonType)
  {
    case ezEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged:
    {
      SetValue(ezMath::Clamp((ezInt32)(m_pSettings->GetSkyLightIntensity() * 10.0f), 0, 20));
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged:
    {
      SetValue(ezMath::Clamp((ezInt32)(m_pSettings->GetDirectionalLightAngle().GetDegree()), 0, 360));
    }
    break;
    case ezEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged:
    {
      SetValue(ezMath::Clamp((ezInt32)(m_pSettings->GetDirectionalLightIntensity() * 10.0f), 1, 200));
    }
    break;
    default:
      break;
  }
}
