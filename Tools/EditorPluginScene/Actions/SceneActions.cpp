#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <QProcess>
#include <Foundation/Logging/Log.h>
#include <Preferences/ScenePreferences.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezSceneActions::s_hSceneCategory;
ezActionDescriptorHandle ezSceneActions::s_hExportScene;
ezActionDescriptorHandle ezSceneActions::s_hRunScene;
ezActionDescriptorHandle ezSceneActions::s_hGameModeSimulate;
ezActionDescriptorHandle ezSceneActions::s_hRenderSelectionOverlay;
ezActionDescriptorHandle ezSceneActions::s_hRenderVisualizers;
ezActionDescriptorHandle ezSceneActions::s_hRenderShapeIcons;
ezActionDescriptorHandle ezSceneActions::s_hRenderGrid;
ezActionDescriptorHandle ezSceneActions::s_hAddAmbientLight;
ezActionDescriptorHandle ezSceneActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezSceneActions::s_hSimulationSpeed[10];
ezActionDescriptorHandle ezSceneActions::s_hGameModePlay;
ezActionDescriptorHandle ezSceneActions::s_hGameModeStop;
ezActionDescriptorHandle ezSceneActions::s_hCameraSpeed;

void ezSceneActions::RegisterActions()
{
  s_hSceneCategory = EZ_REGISTER_CATEGORY("SceneCategory");
  s_hExportScene = EZ_REGISTER_ACTION_1("Scene.Export", ezActionScope::Document, "Scene", "Ctrl+E", ezSceneAction, ezSceneAction::ActionType::ExportScene);
  s_hRunScene = EZ_REGISTER_ACTION_1("Scene.Run", ezActionScope::Document, "Scene", "Ctrl+R", ezSceneAction, ezSceneAction::ActionType::RunScene);
  s_hGameModeSimulate = EZ_REGISTER_ACTION_1("Scene.GameMode.Simulate", ezActionScope::Document, "Scene", "F5", ezSceneAction, ezSceneAction::ActionType::StartGameModeSimulate);
  s_hRenderSelectionOverlay = EZ_REGISTER_ACTION_1("Scene.Render.SelectionOverlay", ezActionScope::Document, "Scene", "End", ezSceneAction, ezSceneAction::ActionType::RenderSelectionOverlay);
  s_hRenderVisualizers = EZ_REGISTER_ACTION_1("Scene.Render.Visualizers", ezActionScope::Document, "Scene", "Insert", ezSceneAction, ezSceneAction::ActionType::RenderVisualizers);
  s_hRenderShapeIcons = EZ_REGISTER_ACTION_1("Scene.Render.ShapeIcons", ezActionScope::Document, "Scene", "Space", ezSceneAction, ezSceneAction::ActionType::RenderShapeIcons);
  s_hRenderGrid = EZ_REGISTER_ACTION_1("Scene.Render.Grid", ezActionScope::Document, "Scene", "Home", ezSceneAction, ezSceneAction::ActionType::RenderGrid);
  s_hAddAmbientLight = EZ_REGISTER_ACTION_1("Scene.Render.AddAmbient", ezActionScope::Document, "Scene", "", ezSceneAction, ezSceneAction::ActionType::AddAmbientLight);
  s_hGameModePlay = EZ_REGISTER_ACTION_1("Scene.GameMode.Play", ezActionScope::Document, "Scene", "Ctrl+F5", ezSceneAction, ezSceneAction::ActionType::StartGameModePlay);
  s_hGameModeStop = EZ_REGISTER_ACTION_1("Scene.GameMode.Stop", ezActionScope::Document, "Scene", "Shift+F5", ezSceneAction, ezSceneAction::ActionType::StopGameMode);

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
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.10", ezActionScope::Document, "Simulation - Speed", "", ezSceneAction, ezSceneAction::ActionType::SimulationSpeed, 10.0f);

  s_hCameraSpeed = EZ_REGISTER_ACTION_1("Scene.Camera.Speed", ezActionScope::Document, "Camera", "", ezSceneSliderAction, ezSceneSliderAction::ActionType::CameraSpeed);
}

void ezSceneActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSceneCategory);
  ezActionManager::UnregisterAction(s_hExportScene);
  ezActionManager::UnregisterAction(s_hRunScene);
  ezActionManager::UnregisterAction(s_hGameModeSimulate);
  ezActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  ezActionManager::UnregisterAction(s_hRenderVisualizers);
  ezActionManager::UnregisterAction(s_hRenderShapeIcons);
  ezActionManager::UnregisterAction(s_hRenderGrid);
  ezActionManager::UnregisterAction(s_hAddAmbientLight);
  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  ezActionManager::UnregisterAction(s_hGameModePlay);
  ezActionManager::UnregisterAction(s_hGameModeStop);
  ezActionManager::UnregisterAction(s_hCameraSpeed);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}


void ezSceneActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("EditorPluginScene_DocumentMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const char* szSubPath = "Menu.Scene/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.Scene", 1.0f);
    pMap->MapAction(s_hExportScene, szSubPath, 1.0f);
    pMap->MapAction(s_hRunScene, szSubPath, 2.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);
    pMap->MapAction(s_hGameModeStop, szSubPath, 4.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 5.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 6.0f);

    ezStringBuilder sSubPath(szSubPath, "/Scene.Simulation.Speed.Menu");

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  }

  {
    const char* szSubPath = "Menu.View/SceneCategory";

    pMap->MapAction(s_hSceneCategory, "Menu.View", 1.0f);
    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 1.0f);
    pMap->MapAction(s_hRenderVisualizers, szSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, szSubPath, 3.0f);
    pMap->MapAction(s_hRenderGrid, szSubPath, 3.05f);
    pMap->MapAction(s_hAddAmbientLight, szSubPath, 3.1f);
    pMap->MapAction(s_hCameraSpeed, szSubPath, 4.0f);
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

    pMap->MapAction(s_hGameModeStop, szSubPath, 1.0f);
    pMap->MapAction(s_hGameModeSimulate, szSubPath, 2.0f);
    pMap->MapAction(s_hGameModePlay, szSubPath, 3.0f);

    pMap->MapAction(s_hRenderSelectionOverlay, szSubPath, 4.0f);
    pMap->MapAction(s_hRenderVisualizers, szSubPath, 5.0f);
    pMap->MapAction(s_hRenderShapeIcons, szSubPath, 6.0f);
    pMap->MapAction(s_hRenderGrid, szSubPath, 6.5f);
    pMap->MapAction(s_hCameraSpeed, szSubPath, 7.0f);
  }
}

ezSceneAction::ezSceneAction(const ezActionContext& context, const char* szName, ezSceneAction::ActionType type, float fSimSpeed) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<ezSceneDocument*>(static_cast<const ezSceneDocument*>(context.m_pDocument));
  m_pSceneDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

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

  case ActionType::RenderSelectionOverlay:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/Selection16.png");
    SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
    break;

  case ActionType::RenderVisualizers:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/Visualizers16.png");
    SetChecked(m_pSceneDocument->GetRenderVisualizers());
    break;

  case ActionType::RenderShapeIcons:
    SetCheckable(true);
    SetIconPath(":/EditorPluginScene/Icons/ShapeIcons16.png");
    SetChecked(m_pSceneDocument->GetRenderShapeIcons());
    break;

  case ActionType::RenderGrid:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);
      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezSceneAction::OnPreferenceChange, this));

      SetCheckable(true);
      SetIconPath(":/EditorPluginScene/Icons/Grid16.png");
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

  case ActionType::AddAmbientLight:
    SetCheckable(true);
    //SetIconPath(":/EditorPluginScene/Icons/ShapeIcons16.png"); // TODO icon
    SetChecked(m_pSceneDocument->GetAddAmbientLight());
    break;

  case ActionType::SimulationSpeed:
    SetCheckable(true);
    SetChecked(m_pSceneDocument->GetSimulationSpeed() == m_fSimSpeed);
    break;

  case ActionType::StartGameModePlay:
    SetIconPath(":/EditorPluginScene/Icons/ScenePlayTheGame16.png");
    break;

  case ActionType::StopGameMode:
    SetIconPath(":/EditorPluginScene/Icons/SceneStop16.png");
    break;
  }

  UpdateState();
}

ezSceneAction::~ezSceneAction()
{
  m_pSceneDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneAction::SceneEventHandler, this));

  switch (m_Type)
  {
  case ActionType::RenderGrid:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezSceneAction::OnPreferenceChange, this));
    }
    break;
  }
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

      const ezStringBuilder sPath = m_pSceneDocument->GetFinalOutputFileName();
      const char* szPath = sPath.GetData();
      arguments << QString::fromUtf8(szPath);

      ezLog::InfoPrintf("Running: Player.exe -scene \"%s\"", sPath.GetData());
      m_pSceneDocument->ShowDocumentStatus("Running: Player.exe -scene \"%s\"", sPath.GetData());

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

  case ActionType::RenderSelectionOverlay:
    m_pSceneDocument->SetRenderSelectionOverlay(!m_pSceneDocument->GetRenderSelectionOverlay());
    return;

  case ActionType::RenderVisualizers:
    m_pSceneDocument->SetRenderVisualizers(!m_pSceneDocument->GetRenderVisualizers());
    return;

  case ActionType::RenderShapeIcons:
    m_pSceneDocument->SetRenderShapeIcons(!m_pSceneDocument->GetRenderShapeIcons());
    return;

  case ActionType::RenderGrid:
    {
      auto pPref = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);
      pPref->SetShowGrid(!pPref->GetShowGrid());
    }
    return;

  case ActionType::AddAmbientLight:
    m_pSceneDocument->SetAddAmbientLight(!m_pSceneDocument->GetAddAmbientLight());
    return;

  case ActionType::SimulationSpeed:
    m_pSceneDocument->SetSimulationSpeed(m_fSimSpeed);
    return;
  }
}

void ezSceneAction::SceneEventHandler(const ezSceneDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocumentEvent::Type::GameModeChanged:
    UpdateState();
    break;

  case ezSceneDocumentEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pSceneDocument->GetRenderSelectionOverlay());
      }
    }
    break;

  case ezSceneDocumentEvent::Type::RenderVisualizersChanged:
    {
      if (m_Type == ActionType::RenderVisualizers)
      {
        SetChecked(m_pSceneDocument->GetRenderVisualizers());
      }
    }
    break;

  case ezSceneDocumentEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pSceneDocument->GetRenderShapeIcons());
      }
    }
    break;

  case ezSceneDocumentEvent::Type::AddAmbientLightChanged:
    {
      if (m_Type == ActionType::AddAmbientLight)
      {
        SetChecked(m_pSceneDocument->GetAddAmbientLight());
      }
    }
    break;

  case ezSceneDocumentEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pSceneDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;
  }
}

void ezSceneAction::UpdateState()
{
  if (m_Type == ActionType::StartGameModeSimulate ||
      m_Type == ActionType::StartGameModePlay ||
      m_Type == ActionType::ExportScene ||
      m_Type == ActionType::RunScene)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() == GameMode::Off);
  }

  if (m_Type == ActionType::StopGameMode)
  {
    SetEnabled(m_pSceneDocument->GetGameMode() != GameMode::Off);
  }
}


void ezSceneAction::OnPreferenceChange(ezPreferences* pref)
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

  switch (m_Type)
  {
  case ActionType::RenderGrid:
    {
      SetChecked(pPreferences->GetShowGrid());
    }
    break;
  }
}

ezSceneSliderAction::ezSceneSliderAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezSliderAction(context, szName)
{
  m_Type = type;
  m_pSceneDocument = const_cast<ezSceneDocument*>(static_cast<const ezSceneDocument*>(context.m_pDocument));

  switch (m_Type)
  {
  case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezSceneSliderAction::OnPreferenceChange, this));

      SetRange(0, 24);
    }
    break;
  }

  UpdateState();
}

ezSceneSliderAction::~ezSceneSliderAction()
{
  switch (m_Type)
  {
  case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezSceneSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezSceneSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  switch (m_Type)
  {
  case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

      pPreferences->SetCameraSpeed(value.Get<ezInt32>());
    }
    break;
  }
}

void ezSceneSliderAction::OnPreferenceChange(ezPreferences* pref)
{
  UpdateState();
}

void ezSceneSliderAction::UpdateState()
{
  switch (m_Type)
  {
  case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pSceneDocument);

      SetValue(pPreferences->GetCameraSpeed());
    }
    break;
  }
}
