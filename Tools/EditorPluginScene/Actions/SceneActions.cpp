#include <PCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>
#include <QProcess>
#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hSceneUtilsMenu;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;
ezActionDescriptorHandle ezSceneActions::s_hRunScene;
ezActionDescriptorHandle ezSceneActions::s_hGameModeSimulate;
ezActionDescriptorHandle ezSceneActions::s_hGameModePlay;
ezActionDescriptorHandle ezSceneActions::s_hGameModeStop;
ezActionDescriptorHandle ezSceneActions::s_hUtilExportSceneToOBJ;
ezActionDescriptorHandle ezSceneActions::s_hPullObjectEngineState;

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
  s_hGameModeStop = EZ_REGISTER_ACTION_1("Scene.GameMode.Stop", ezActionScope::Document, "Scene", "Shift+F5", ezSceneAction,
                                         ezSceneAction::ActionType::StopGameMode);

  s_hUtilExportSceneToOBJ = EZ_REGISTER_ACTION_1("Scene.ExportSceneToOBJ", ezActionScope::Document, "Scene", "", ezSceneAction,
                                                 ezSceneAction::ActionType::ExportSceneToOBJ);

  s_hPullObjectEngineState = EZ_REGISTER_ACTION_1("Scene.PullObjectEngineState", ezActionScope::Document, "Scene", "Ctrl+K,Ctrl+P", ezSceneAction,
                                                  ezSceneAction::ActionType::PullObjectEngineState);
}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hSceneUtilsMenu);
  ezActionManager::UnregisterAction(s_hExportScene);
  ezActionManager::UnregisterAction(s_hRunScene);
  ezActionManager::UnregisterAction(s_hGameModeSimulate);
  ezActionManager::UnregisterAction(s_hGameModePlay);
  ezActionManager::UnregisterAction(s_hGameModeStop);
  ezActionManager::UnregisterAction(s_hUtilExportSceneToOBJ);
  ezActionManager::UnregisterAction(s_hPullObjectEngineState);
}


void ezSceneActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "Menu.Scene/SceneCategory";
    const char* szUtilsSubPath = "Menu.Scene/Scene.Utils.Menu";

    pMap->MapAction(s_hSceneUtilsMenu, "Menu.Scene", 2.0f);
    pMap->MapAction(s_hPullObjectEngineState, szUtilsSubPath, 1.0f);
    pMap->MapAction(s_hUtilExportSceneToOBJ, szUtilsSubPath, 2.0f);

    pMap->MapAction(s_hSceneCategory, "Menu.Scene", 3.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hRunScene, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);
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

    case ActionType::StopGameMode:
      SetIconPath(":/EditorPluginScene/Icons/SceneStop16.png");
      break;

    case ActionType::ExportSceneToOBJ:
      // SetIconPath(":/EditorPluginScene/Icons/SceneStop16.png"); // TODO: icon
      break;

    case ActionType::PullObjectEngineState:
      SetIconPath(":/EditorPluginScene/Icons/PullObjectState16.png");
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
      m_pSceneDocument->ExportScene();
      return;

    case ActionType::RunScene:
    {
      QStringList arguments;
      arguments << "-scene";

      const ezStringBuilder sPath =
          m_pSceneDocument->GetAssetDocumentManager()->GetAbsoluteOutputFileName(m_pSceneDocument->GetDocumentPath(), "");

      const char* szPath = sPath.GetData();
      arguments << QString::fromUtf8(szPath);

      ezStringBuilder sWndCfgPath = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
      sWndCfgPath.AppendPath("Window.ddl");

      if (ezOSFile::ExistsFile(sWndCfgPath))
      {
        arguments << "-wnd";
        arguments << QString::fromUtf8(sWndCfgPath);
      }

      ezLog::Info("Running: Player.exe -scene \"{0}\"", sPath);
      m_pSceneDocument->ShowDocumentStatus(ezFmt("Running: Player.exe -scene \"{0}\"", sPath));

      QProcess proc;
      proc.startDetached(QString::fromUtf8("Player.exe"), arguments);
    }
      return;

    case ActionType::StartGameModePlay:
      m_pSceneDocument->TriggerGameModePlay();
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

    case ActionType::PullObjectEngineState:
    {
      ezPullObjectStateMsgToEngine msg;
      m_pSceneDocument->SendMessageToEngine(&msg);
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

  if (m_Type == ActionType::PullObjectEngineState)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off && !m_pSceneDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
