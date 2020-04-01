#include <EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#include <QFileDialog>
#include <QProcess>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hSceneUtilsMenu;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;
ezActionDescriptorHandle ezSceneActions::s_hRunScene;
ezActionDescriptorHandle ezSceneActions::s_hGameModeSimulate;
ezActionDescriptorHandle ezSceneActions::s_hGameModePlay;
ezActionDescriptorHandle ezSceneActions::s_hGameModePlayFromHere;
ezActionDescriptorHandle ezSceneActions::s_hGameModeStop;
ezActionDescriptorHandle ezSceneActions::s_hUtilExportSceneToOBJ;
ezActionDescriptorHandle ezSceneActions::s_hKeepSimulationChanges;
ezActionDescriptorHandle ezSceneActions::s_hCreateThumbnail;
ezActionDescriptorHandle ezSceneActions::s_hFavouriteCamsMenu;
ezActionDescriptorHandle ezSceneActions::s_hStoreEditorCamera[10];
ezActionDescriptorHandle ezSceneActions::s_hRestoreEditorCamera[10];
ezActionDescriptorHandle ezSceneActions::s_hJumpToCamera[10];
ezActionDescriptorHandle ezSceneActions::s_hCreateLevelCamera[10];

void ezSceneActions::RegisterActions()
{
  s_hSceneCategory = EZ_REGISTER_CATEGORY("SceneCategory");
  s_hSceneUtilsMenu = EZ_REGISTER_MENU_WITH_ICON("Scene.Utils.Menu", "");

  s_hExportScene = EZ_REGISTER_ACTION_1("Scene.Export", ezActionScope::Document, "Scene", "Ctrl+E", ezSceneAction,
    ezSceneAction::ActionType::ExportScene);
  s_hRunScene =
    EZ_REGISTER_ACTION_1("Scene.Run", ezActionScope::Document, "Scene", "Ctrl+R", ezSceneAction, ezSceneAction::ActionType::RunScene);
  s_hGameModeSimulate = EZ_REGISTER_ACTION_1("Scene.GameMode.Simulate", ezActionScope::Document, "Scene", "F5", ezSceneAction,
    ezSceneAction::ActionType::StartGameModeSimulate);
  s_hGameModePlay = EZ_REGISTER_ACTION_1("Scene.GameMode.Play", ezActionScope::Document, "Scene", "Ctrl+F5", ezSceneAction,
    ezSceneAction::ActionType::StartGameModePlay);

  s_hGameModePlayFromHere = EZ_REGISTER_ACTION_1("Scene.GameMode.PlayFromHere", ezActionScope::Document, "Scene", "Ctrl+Shift+F5",
    ezSceneAction, ezSceneAction::ActionType::StartGameModePlayFromHere);

  s_hGameModeStop = EZ_REGISTER_ACTION_1("Scene.GameMode.Stop", ezActionScope::Document, "Scene", "Shift+F5", ezSceneAction,
    ezSceneAction::ActionType::StopGameMode);

  s_hUtilExportSceneToOBJ = EZ_REGISTER_ACTION_1("Scene.ExportSceneToOBJ", ezActionScope::Document, "Scene", "", ezSceneAction,
    ezSceneAction::ActionType::ExportSceneToOBJ);

  s_hKeepSimulationChanges = EZ_REGISTER_ACTION_1("Scene.KeepSimulationChanges", ezActionScope::Document, "Scene", "K", ezSceneAction,
    ezSceneAction::ActionType::KeepSimulationChanges);

  s_hCreateThumbnail = EZ_REGISTER_ACTION_1("Scene.CreateThumbnail", ezActionScope::Document, "Scene", "", ezSceneAction,
    ezSceneAction::ActionType::CreateThumbnail);
  // unfortunately the macros use lambdas thus using a loop to generate the strings does not work
  {
    s_hFavouriteCamsMenu = EZ_REGISTER_MENU_WITH_ICON("Scene.FavouriteCams.Menu", "");

    s_hStoreEditorCamera[0] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.0", ezActionScope::Document, "Scene - Cameras", "Ctrl+0",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera0);
    s_hStoreEditorCamera[1] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.1", ezActionScope::Document, "Scene - Cameras", "Ctrl+1",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera1);
    s_hStoreEditorCamera[2] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.2", ezActionScope::Document, "Scene - Cameras", "Ctrl+2",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera2);
    s_hStoreEditorCamera[3] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.3", ezActionScope::Document, "Scene - Cameras", "Ctrl+3",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera3);
    s_hStoreEditorCamera[4] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.4", ezActionScope::Document, "Scene - Cameras", "Ctrl+4",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera4);
    s_hStoreEditorCamera[5] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.5", ezActionScope::Document, "Scene - Cameras", "Ctrl+5",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera5);
    s_hStoreEditorCamera[6] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.6", ezActionScope::Document, "Scene - Cameras", "Ctrl+6",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera6);
    s_hStoreEditorCamera[7] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.7", ezActionScope::Document, "Scene - Cameras", "Ctrl+7",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera7);
    s_hStoreEditorCamera[8] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.8", ezActionScope::Document, "Scene - Cameras", "Ctrl+8",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera8);
    s_hStoreEditorCamera[9] = EZ_REGISTER_ACTION_1("Scene.Camera.Store.9", ezActionScope::Document, "Scene - Cameras", "Ctrl+9",
      ezSceneAction, ezSceneAction::ActionType::StoreEditorCamera9);

    s_hRestoreEditorCamera[0] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.0", ezActionScope::Document, "Scene - Cameras", "0",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera0);
    s_hRestoreEditorCamera[1] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.1", ezActionScope::Document, "Scene - Cameras", "1",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera1);
    s_hRestoreEditorCamera[2] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.2", ezActionScope::Document, "Scene - Cameras", "2",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera2);
    s_hRestoreEditorCamera[3] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.3", ezActionScope::Document, "Scene - Cameras", "3",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera3);
    s_hRestoreEditorCamera[4] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.4", ezActionScope::Document, "Scene - Cameras", "4",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera4);
    s_hRestoreEditorCamera[5] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.5", ezActionScope::Document, "Scene - Cameras", "5",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera5);
    s_hRestoreEditorCamera[6] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.6", ezActionScope::Document, "Scene - Cameras", "6",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera6);
    s_hRestoreEditorCamera[7] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.7", ezActionScope::Document, "Scene - Cameras", "7",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera7);
    s_hRestoreEditorCamera[8] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.8", ezActionScope::Document, "Scene - Cameras", "8",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera8);
    s_hRestoreEditorCamera[9] = EZ_REGISTER_ACTION_1("Scene.Camera.Restore.9", ezActionScope::Document, "Scene - Cameras", "9",
      ezSceneAction, ezSceneAction::ActionType::RestoreEditorCamera9);

    s_hJumpToCamera[0] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.0", ezActionScope::Document, "Scene - Cameras", "Alt+0", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera0);
    s_hJumpToCamera[1] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.1", ezActionScope::Document, "Scene - Cameras", "Alt+1", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera1);
    s_hJumpToCamera[2] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.2", ezActionScope::Document, "Scene - Cameras", "Alt+2", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera2);
    s_hJumpToCamera[3] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.3", ezActionScope::Document, "Scene - Cameras", "Alt+3", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera3);
    s_hJumpToCamera[4] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.4", ezActionScope::Document, "Scene - Cameras", "Alt+4", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera4);
    s_hJumpToCamera[5] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.5", ezActionScope::Document, "Scene - Cameras", "Alt+5", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera5);
    s_hJumpToCamera[6] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.6", ezActionScope::Document, "Scene - Cameras", "Alt+6", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera6);
    s_hJumpToCamera[7] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.7", ezActionScope::Document, "Scene - Cameras", "Alt+7", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera7);
    s_hJumpToCamera[8] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.8", ezActionScope::Document, "Scene - Cameras", "Alt+8", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera8);
    s_hJumpToCamera[9] = EZ_REGISTER_ACTION_1("Scene.Camera.JumpTo.9", ezActionScope::Document, "Scene - Cameras", "Alt+9", ezSceneAction,
      ezSceneAction::ActionType::JumpToCamera9);

    s_hCreateLevelCamera[0] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.0", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+0",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera0);
    s_hCreateLevelCamera[1] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.1", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+1",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera1);
    s_hCreateLevelCamera[2] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.2", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+2",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera2);
    s_hCreateLevelCamera[3] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.3", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+3",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera3);
    s_hCreateLevelCamera[4] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.4", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+4",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera4);
    s_hCreateLevelCamera[5] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.5", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+5",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera5);
    s_hCreateLevelCamera[6] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.6", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+6",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera6);
    s_hCreateLevelCamera[7] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.7", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+7",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera7);
    s_hCreateLevelCamera[8] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.8", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+8",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera8);
    s_hCreateLevelCamera[9] = EZ_REGISTER_ACTION_1("Scene.Camera.Create.9", ezActionScope::Document, "Scene - Cameras", "Ctrl+Alt+9",
      ezSceneAction, ezSceneAction::ActionType::CreateLevelCamera9);
  }
}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hSceneUtilsMenu);
  ezActionManager::UnregisterAction(s_hExportScene);
  ezActionManager::UnregisterAction(s_hRunScene);
  ezActionManager::UnregisterAction(s_hGameModeSimulate);
  ezActionManager::UnregisterAction(s_hGameModePlay);
  ezActionManager::UnregisterAction(s_hGameModePlayFromHere);
  ezActionManager::UnregisterAction(s_hGameModeStop);
  ezActionManager::UnregisterAction(s_hUtilExportSceneToOBJ);
  ezActionManager::UnregisterAction(s_hKeepSimulationChanges);
  ezActionManager::UnregisterAction(s_hCreateThumbnail);
  ezActionManager::UnregisterAction(s_hFavouriteCamsMenu);

  for (ezUInt32 i = 0; i < 10; ++i)
  {
    ezActionManager::UnregisterAction(s_hStoreEditorCamera[i]);
    ezActionManager::UnregisterAction(s_hRestoreEditorCamera[i]);
    ezActionManager::UnregisterAction(s_hJumpToCamera[i]);
    ezActionManager::UnregisterAction(s_hCreateLevelCamera[i]);
  }
}

void ezSceneActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "Menu.Scene/SceneCategory";
    const char* szUtilsSubPath = "Menu.Scene/Scene.Utils.Menu";

    pMap->MapAction(s_hSceneUtilsMenu, "Menu.Scene", 2.0f);
    pMap->MapAction(s_hCreateThumbnail, szUtilsSubPath, 0.0f);
    pMap->MapAction(s_hKeepSimulationChanges, szUtilsSubPath, 1.0f);
    pMap->MapAction(s_hUtilExportSceneToOBJ, szUtilsSubPath, 2.0f);

    pMap->MapAction(s_hFavouriteCamsMenu, "Menu.Scene", 3.0f);
    const char* szFavCamsSubPath = "Menu.Scene/Scene.FavouriteCams.Menu";

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      pMap->MapAction(s_hStoreEditorCamera[i], szFavCamsSubPath, 10.0f + i);
      pMap->MapAction(s_hRestoreEditorCamera[i], szFavCamsSubPath, 20.0f + i);
      pMap->MapAction(s_hJumpToCamera[i], szFavCamsSubPath, 30.0f + i);
      pMap->MapAction(s_hCreateLevelCamera[i], szFavCamsSubPath, 40.0f + i);
    }

    pMap->MapAction(s_hSceneCategory, "Menu.Scene", 4.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hRunScene, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);
    pMap->MapAction(s_hGameModePlayFromHere, szSubPath, 7.0f);
  }
}

void ezSceneActions::MapToolbarActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentToolBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";

    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 11.0f);

    pMap->MapAction(s_hGameModeStop, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 3.0f);
  }
}

void ezSceneActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SceneCategory");

  pMap->MapAction(s_hGameModePlayFromHere, szPath, 1.0f);
}

ezSceneAction::ezSceneAction(const ezActionContext& context, const char* szName, ezSceneAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<ezSceneDocument*>(static_cast<const ezSceneDocument*>(context.m_pDocument));
  m_pSceneDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::ExportScene:
      SetIconPath(":/EditorPluginScene/Icons/SceneExport16.png");
      break;

    case ActionType::RunScene:
      SetIconPath(":/EditorPluginScene/Icons/SceneRun16.png");
      break;

    case ActionType::StartGameModeSimulate:
      SetCheckable(true);
      SetIconPath(":/EditorPluginScene/Icons/ScenePlay16.png");
      SetChecked(m_pSceneDocument->GetGameMode() == GameMode::Simulate);
      SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Play);
      break;

    case ActionType::StartGameModePlay:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame16.png");
      break;

    case ActionType::StartGameModePlayFromHere:
      SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame16.png"); // TODO: icon
      break;

    case ActionType::StopGameMode:
      SetIconPath(":/EditorPluginScene/Icons/SceneStop16.png");
      break;

    case ActionType::ExportSceneToOBJ:
      // SetIconPath(":/EditorPluginScene/Icons/SceneStop16.png"); // TODO: icon
      break;

    case ActionType::KeepSimulationChanges:
      SetIconPath(":/EditorPluginScene/Icons/PullObjectState16.png");
      break;

    case ActionType::CreateThumbnail:
      // SetIconPath(":/EditorPluginScene/Icons/PullObjectState16.png"); // TODO: icon
      break;

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
      SetIconPath(":/TypeIcons/ezCameraComponent.png");
      break;
  }

  UpdateState();
}

ezSceneAction::~ezSceneAction()
{
  m_pSceneDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));
}

void ezSceneAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ExportScene:
    {
      m_pSceneDocument->ExportScene(false);

      ezGameObjectDocumentEvent e;
      e.m_Type = ezGameObjectDocumentEvent::Type::GameMode_StartingExternal;
      e.m_pDocument = m_pSceneDocument;
      m_pSceneDocument->s_GameObjectDocumentEvents.Broadcast(e);

      return;
    }

    case ActionType::RunScene:
    {
      QStringList arguments;
      arguments << "-scene";

      const ezStringBuilder sPath = m_pSceneDocument->GetAssetDocumentManager()->GetAbsoluteOutputFileName(m_pSceneDocument->GetAssetDocumentTypeDescriptor(), m_pSceneDocument->GetDocumentPath(), "");

      const char* szPath = sPath.GetData();
      arguments << QString::fromUtf8(szPath);

      ezStringBuilder sWndCfgPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
      sWndCfgPath.AppendPath("Window.ddl");

      if (ezOSFile::ExistsFile(sWndCfgPath))
      {
        arguments << "-wnd";
        arguments << QString::fromUtf8(sWndCfgPath);
      }

      arguments << "-profile";
      arguments << ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

      ezLog::Info("Running: Player.exe -scene \"{0}\"", sPath);
      m_pSceneDocument->ShowDocumentStatus(ezFmt("Running: Player.exe -scene \"{0}\"", sPath));

      QProcess proc;
      proc.startDetached(QString::fromUtf8("Player.exe"), arguments);
      return;
    }

    case ActionType::StartGameModePlay:
      m_pSceneDocument->TriggerGameModePlay(false);
      return;

    case ActionType::StartGameModePlayFromHere:
      m_pSceneDocument->TriggerGameModePlay(true);
      return;

    case ActionType::StartGameModeSimulate:
      m_pSceneDocument->StartSimulateWorld();
      return;

    case ActionType::StopGameMode:
      m_pSceneDocument->StopGameMode();
      return;

    case ActionType::ExportSceneToOBJ:
    {
      ezQtExtractGeometryDlg dlg(nullptr);
      if (dlg.exec() == QDialog::Accepted)
      {
        m_pSceneDocument->ExportSceneGeometry(dlg.s_sDestinationFile.toUtf8().data(), dlg.s_bOnlySelection, dlg.s_iExtractionMode,
          dlg.GetCoordinateSystemTransform());
      }
      return;
    }

    case ActionType::KeepSimulationChanges:
    {
      ezPullObjectStateMsgToEngine msg;
      m_pSceneDocument->SendMessageToEngine(&msg);
      return;
    }

    case ActionType::CreateThumbnail:
      m_pSceneDocument->ExportScene(true);
      return;

    case ActionType::StoreEditorCamera0:
    case ActionType::StoreEditorCamera1:
    case ActionType::StoreEditorCamera2:
    case ActionType::StoreEditorCamera3:
    case ActionType::StoreEditorCamera4:
    case ActionType::StoreEditorCamera5:
    case ActionType::StoreEditorCamera6:
    case ActionType::StoreEditorCamera7:
    case ActionType::StoreEditorCamera8:
    case ActionType::StoreEditorCamera9:
    {
      const ezInt32 iCamIdx = (int)m_Type - (int)ActionType::StoreEditorCamera0;

      m_pSceneDocument->StoreFavouriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(ezFmt("Stored favourite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::RestoreEditorCamera0:
    case ActionType::RestoreEditorCamera1:
    case ActionType::RestoreEditorCamera2:
    case ActionType::RestoreEditorCamera3:
    case ActionType::RestoreEditorCamera4:
    case ActionType::RestoreEditorCamera5:
    case ActionType::RestoreEditorCamera6:
    case ActionType::RestoreEditorCamera7:
    case ActionType::RestoreEditorCamera8:
    case ActionType::RestoreEditorCamera9:
    {
      const ezInt32 iCamIdx = (int)m_Type - (int)ActionType::RestoreEditorCamera0;

      m_pSceneDocument->RestoreFavouriteCamera(iCamIdx);
      m_pSceneDocument->ShowDocumentStatus(ezFmt("Restored favourite camera position {0}", iCamIdx));

      return;
    }

    case ActionType::JumpToCamera0:
    case ActionType::JumpToCamera1:
    case ActionType::JumpToCamera2:
    case ActionType::JumpToCamera3:
    case ActionType::JumpToCamera4:
    case ActionType::JumpToCamera5:
    case ActionType::JumpToCamera6:
    case ActionType::JumpToCamera7:
    case ActionType::JumpToCamera8:
    case ActionType::JumpToCamera9:
    {
      const ezInt32 iCamIdx = (int)m_Type - (int)ActionType::JumpToCamera0;

      const bool bImmediate = value.IsA<bool>() ? value.Get<bool>() : false;
      if (m_pSceneDocument->JumpToLevelCamera(iCamIdx, bImmediate).Failed())
      {
        m_pSceneDocument->ShowDocumentStatus(ezFmt("No Camera Component found with shortcut set to '{0}'", iCamIdx));
      }
      return;
    }

    case ActionType::CreateLevelCamera0:
    case ActionType::CreateLevelCamera1:
    case ActionType::CreateLevelCamera2:
    case ActionType::CreateLevelCamera3:
    case ActionType::CreateLevelCamera4:
    case ActionType::CreateLevelCamera5:
    case ActionType::CreateLevelCamera6:
    case ActionType::CreateLevelCamera7:
    case ActionType::CreateLevelCamera8:
    case ActionType::CreateLevelCamera9:
    {
      const ezInt32 iCamIdx = (int)m_Type - (int)ActionType::CreateLevelCamera0;

      if (m_pSceneDocument->CreateLevelCamera(iCamIdx).Succeeded())
      {
        m_pSceneDocument->ShowDocumentStatus(ezFmt("Create level camera with shortcut set to '{0}'", iCamIdx));
      }
      else
      {
        m_pSceneDocument->ShowDocumentStatus(ezFmt("Could not level camera.", iCamIdx));
      }
      return;
    }
  }
}

void ezSceneAction::SceneEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectEvent::Type::GameModeChanged:
      UpdateState();
      break;
  }
}

void ezSceneAction::UpdateState()
{
  if (m_Type == ActionType::StartGameModeSimulate || m_Type == ActionType::StartGameModePlay || m_Type == ActionType::ExportScene ||
      m_Type == ActionType::RunScene)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() == GameMode::Off);
  }

  if (m_Type == ActionType::StopGameMode)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);
  }

  if (m_Type == ActionType::KeepSimulationChanges)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off && !m_pSceneDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
