#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <QProcess>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hUpdatePrefabs;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;
ezActionDescriptorHandle ezSceneActions::s_hRunScene;
ezActionDescriptorHandle ezSceneActions::s_hEnableWorldSimulation;
ezActionDescriptorHandle ezSceneActions::s_hRenderSelectionOverlay;
ezActionDescriptorHandle ezSceneActions::s_hRenderShapeIcons;
ezActionDescriptorHandle ezSceneActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezSceneActions::s_hSimulationSpeed[10];
ezActionDescriptorHandle ezSceneActions::s_hPlayTheGame;

void ezSceneActions::RegisterActions()
{
  s_hSceneCategory = EZ_REGISTER_CATEGORY("SceneCategory");
  s_hUpdatePrefabs = EZ_REGISTER_ACTION_1("Prefabs.UpdateAll", ezActionScope::Document, "Scene", "Ctrl+Shift+P", ezSceneAction, ezSceneAction::ActionType::UpdatePrefabs);
  s_hExportScene = EZ_REGISTER_ACTION_1("Scene.Export", ezActionScope::Document, "Scene", "Ctrl+E", ezSceneAction, ezSceneAction::ActionType::ExportScene);
  s_hRunScene = EZ_REGISTER_ACTION_1("Scene.Run", ezActionScope::Document, "Scene", "Ctrl+R", ezSceneAction, ezSceneAction::ActionType::RunScene);
  s_hEnableWorldSimulation = EZ_REGISTER_ACTION_1("Scene.SimulateWorld", ezActionScope::Document, "Scene", "Ctrl+F5", ezSceneAction, ezSceneAction::ActionType::SimulateWorld);
  s_hRenderSelectionOverlay = EZ_REGISTER_ACTION_1( "Scene.Render.SelectionOverlay", ezActionScope::Document, "Scene", "Ctrl+M", ezSceneAction, ezSceneAction::ActionType::RenderSelectionOverlay);
  s_hRenderShapeIcons = EZ_REGISTER_ACTION_1("Scene.Render.ShapeIcons", ezActionScope::Document, "Scene", "Space", ezSceneAction, ezSceneAction::ActionType::RenderShapeIcons);
  s_hPlayTheGame = EZ_REGISTER_ACTION_1( "Scene.PlayTheGame", ezActionScope::Document, "Scene", "Ctrl+Shift+F5", ezSceneAction, ezSceneAction::ActionType::PlayTheGame);

  s_hSimulationSpeedMenu = EZ_REGISTER_MENU_WITH_ICON("Scene.Simulation.Speed.Menu", "");
  s_hSimulationSpeed[0] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.01", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.025", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.05", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.1", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.15", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.2", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.3", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.4", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.5", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.10", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed,10.0f);

}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hUpdatePrefabs);
  ezActionManager::UnregisterAction(s_hExportScene);
  ezActionManager::UnregisterAction(s_hRunScene);
  ezActionManager::UnregisterAction(s_hEnableWorldSimulation);
  ezActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  ezActionManager::UnregisterAction(s_hRenderShapeIcons);
  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  ezActionManager::UnregisterAction(s_hPlayTheGame);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}


void ezSceneActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "Menu.Tools/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.Tools", 6.0f);
    pMap->MapAction(s_hUpdatePrefabs, szSubPath, 1.0f);
  }

  {
    const char* szSubPath = "Menu.Scene/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.Scene", 1.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hRunScene, szSubPath, 2.0f);
    pMap->MapAction(s_hEnableWorldSimulation, szSubPath, 3.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 4.0f);
	pMap->MapAction(s_hPlayTheGame, szSubPath, 5.0f);

    ezStringBuilder sSubPath(szSubPath, "/Scene.Simulation.Speed.Menu");

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  }

  {
    const char* szSubPath = "Menu.View/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.View", 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 1.0f);
    pMap->MapAction(s_hRenderShapeIcons, szSubPath, 2.0f);
  }
}


void ezSceneActions::MapToolbarActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentToolBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "SceneCategory";

    /// \todo This works incorrectly with value 6.0f -> it places the action inside the snap category
    pMap->MapAction(s_hSceneCategory, "", 7.0f);

    pMap->MapAction(s_hEnableWorldSimulation, szSubPath, 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, szSubPath, 3.0f);
  }
}

ezSceneAction::ezSceneAction(const ezActionContext& context, const char* szName, ezSceneAction::ActionType type, float fSimSpeed) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    SetIconPath(":/EditorPluginScene/PrefabUpdate.png");
    break;

  case ActionType::ExportScene:
    SetIconPath(":/EditorPluginScene/Icons/SceneExport16.png");
    break;

  case ActionType::RunScene:
    SetIconPath(":/EditorPluginScene/Icons/SceneRun16.png");
    break;

  case ActionType::SimulateWorld:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/ScenePlay16.png");
    SetChecked(m_pSceneDocument->GetSimulateWorld());
    break;

  case ActionType::RenderSelectionOverlay:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/Selection16.png");
    SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
    break;

  case ActionType::RenderShapeIcons:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/ShapeIcons16.png");
    SetChecked(m_pSceneDocument->GetRenderShapeIcons());
    break;

  case ActionType::SimulationSpeed:
    SetCheckable(true);
    SetChecked(m_pSceneDocument->GetSimulationSpeed() == m_fSimSpeed);
    break;

  case ActionType::PlayTheGame:
    SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame16.png");
    break;
  }
}

ezSceneAction::~ezSceneAction()
{
  m_pSceneDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));
}

void ezSceneAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::UpdatePrefabs:
    m_pSceneDocument->UpdatePrefabs();
    return;

  case ActionType::ExportScene:
    m_pSceneDocument->TriggerExportScene();
    return;

  case ActionType::RunScene:
    {
      QStringList arguments;
      arguments << "-scene";
      const ezStringBuilder sPath = m_pSceneDocument->GetBinaryTargetFile();
      const char* szPath = sPath.GetData();
      arguments << QString::fromUtf8(szPath);

      ezLog::Info("Running: Player.exe -scene \"%s\"", sPath.GetData());

      QProcess proc;
      proc.startDetached(QString::fromUtf8("Player.exe"), arguments);
    }
    return;

  case ActionType::PlayTheGame:
    m_pSceneDocument->TriggerPlayTheGame();
    return;

  case ActionType::SimulateWorld:
    {
      m_pSceneDocument->SetSimulateWorld(!m_pSceneDocument->GetSimulateWorld());
    }
    return;

  case ActionType::RenderSelectionOverlay:
    {
      m_pSceneDocument->SetRenderSelectionOverlay(!m_pSceneDocument->GetRenderSelectionOverlay());
    }
    return;

  case ActionType::RenderShapeIcons:
    {
      m_pSceneDocument->SetRenderShapeIcons(!m_pSceneDocument->GetRenderShapeIcons());
    }
    return;

  case ActionType::SimulationSpeed:
    {
      m_pSceneDocument->SetSimulationSpeed(m_fSimSpeed);
    }
    return;
  }
}

void ezSceneAction::SceneEventHandler(const ezSceneDocument::SceneEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocument::SceneEvent::Type::SimulateModeChanged:
    {
      if (m_Type == ActionType::SimulateWorld)
      {
        SetChecked(m_pSceneDocument->GetSimulateWorld());
      }
    }
    break;

  case ezSceneDocument::SceneEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
      }
    }
    break;

  case ezSceneDocument::SceneEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pSceneDocument->GetRenderShapeIcons());
      }
    }
    break;
  case ezSceneDocument::SceneEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pSceneDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;
  }
}

