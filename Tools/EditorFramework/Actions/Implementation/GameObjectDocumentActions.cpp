#include <PCH.h>

#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QProcess>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectDocumentAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraSpeedSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezGameObjectDocumentActions::s_hGameObjectCategory;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hRenderSelectionOverlay;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hRenderVisualizers;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hRenderShapeIcons;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hRenderGrid;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hAddAmbientLight;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hSimulationSpeed[10];
ezActionDescriptorHandle ezGameObjectDocumentActions::s_hCameraSpeed;

void ezGameObjectDocumentActions::RegisterActions()
{
  s_hGameObjectCategory = EZ_REGISTER_CATEGORY("GameObjectCategory");
  s_hRenderSelectionOverlay =
      EZ_REGISTER_ACTION_1("Scene.Render.SelectionOverlay", ezActionScope::Document, "Scene", "End", ezGameObjectDocumentAction,
                           ezGameObjectDocumentAction::ActionType::RenderSelectionOverlay);
  s_hRenderVisualizers = EZ_REGISTER_ACTION_1("Scene.Render.Visualizers", ezActionScope::Document, "Scene", "Insert",
                                              ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::RenderVisualizers);
  s_hRenderShapeIcons = EZ_REGISTER_ACTION_1("Scene.Render.ShapeIcons", ezActionScope::Document, "Scene", "Enter",
                                             ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::RenderShapeIcons);
  s_hRenderGrid = EZ_REGISTER_ACTION_1("Scene.Render.Grid", ezActionScope::Document, "Scene", "Home", ezGameObjectDocumentAction,
                                       ezGameObjectDocumentAction::ActionType::RenderGrid);
  s_hAddAmbientLight = EZ_REGISTER_ACTION_1("Scene.Render.AddAmbient", ezActionScope::Document, "Scene", "", ezGameObjectDocumentAction,
                                            ezGameObjectDocumentAction::ActionType::AddAmbientLight);

  s_hSimulationSpeedMenu = EZ_REGISTER_MENU_WITH_ICON("Scene.Simulation.Speed.Menu", "");
  s_hSimulationSpeed[0] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.01", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.025", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.05", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.1", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.15", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.2", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.3", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.4", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.5", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("Scene.Simulation.Speed.10", ezActionScope::Document, "Simulation - Speed", "",
                                               ezGameObjectDocumentAction, ezGameObjectDocumentAction::ActionType::SimulationSpeed, 10.0f);

  s_hCameraSpeed = EZ_REGISTER_ACTION_1("Scene.Camera.Speed", ezActionScope::Document, "Camera", "", ezCameraSpeedSliderAction,
                                        ezCameraSpeedSliderAction::ActionType::CameraSpeed);
}

void ezGameObjectDocumentActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGameObjectCategory);
  ezActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  ezActionManager::UnregisterAction(s_hRenderVisualizers);
  ezActionManager::UnregisterAction(s_hRenderShapeIcons);
  ezActionManager::UnregisterAction(s_hRenderGrid);
  ezActionManager::UnregisterAction(s_hAddAmbientLight);

  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);

  ezActionManager::UnregisterAction(s_hCameraSpeed);
}

void ezGameObjectDocumentActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, szPath, 0.9f);

    ezStringBuilder sSubPath(szPath, "/GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 1.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 3.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 3.05f);
    pMap->MapAction(s_hAddAmbientLight, sSubPath, 3.1f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 4.0f);
  }
}

void ezGameObjectDocumentActions::MapMenuSimulationSpeed(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    ezStringBuilder sSubPath(szPath, "/GameObjectCategory");

    pMap->MapAction(s_hGameObjectCategory, szPath, 1.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, sSubPath, 3.0f);

    ezStringBuilder sSubPathSim(sSubPath, "/Scene.Simulation.Speed.Menu");
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], sSubPathSim, i + 1.0f);
  }
}

void ezGameObjectDocumentActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, szPath, 12.0f);

    ezStringBuilder sSubPath(szPath, "/GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 4.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 5.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 6.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 6.5f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

ezGameObjectDocumentAction::ezGameObjectDocumentAction(const ezActionContext& context, const char* szName,
                                                       ezGameObjectDocumentAction::ActionType type, float fSimSpeed)
    : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pGameObjectDocument = const_cast<ezGameObjectDocument*>(static_cast<const ezGameObjectDocument*>(context.m_pDocument));
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectDocumentAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Selection16.png");
      SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers16.png");
      SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      break;

    case ActionType::RenderShapeIcons:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/ShapeIcons16.png");
      SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      break;

    case ActionType::RenderGrid:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);
      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezGameObjectDocumentAction::OnPreferenceChange, this));

      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid16.png");
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    case ActionType::AddAmbientLight:
      SetCheckable(true);
      // SetIconPath(":/EditorPluginScene/Icons/ShapeIcons16.png"); // TODO icon
      SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      break;

    case ActionType::SimulationSpeed:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      break;
  }
}

ezGameObjectDocumentAction::~ezGameObjectDocumentAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectDocumentAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezGameObjectDocumentAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezGameObjectDocumentAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      m_pGameObjectDocument->SetRenderSelectionOverlay(!m_pGameObjectDocument->GetRenderSelectionOverlay());
      return;

    case ActionType::RenderVisualizers:
      m_pGameObjectDocument->SetRenderVisualizers(!m_pGameObjectDocument->GetRenderVisualizers());
      return;

    case ActionType::RenderShapeIcons:
      m_pGameObjectDocument->SetRenderShapeIcons(!m_pGameObjectDocument->GetRenderShapeIcons());
      return;

    case ActionType::RenderGrid:
    {
      auto pPref = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);
      pPref->SetShowGrid(!pPref->GetShowGrid());
    }
      return;

    case ActionType::AddAmbientLight:
      m_pGameObjectDocument->SetAddAmbientLight(!m_pGameObjectDocument->GetAddAmbientLight());
      return;

    case ActionType::SimulationSpeed:
      m_pGameObjectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;
  }
}

void ezGameObjectDocumentAction::SceneEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      }
    }
    break;

    case ezGameObjectEvent::Type::RenderVisualizersChanged:
    {
      if (m_Type == ActionType::RenderVisualizers)
      {
        SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      }
    }
    break;

    case ezGameObjectEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      }
    }
    break;

    case ezGameObjectEvent::Type::AddAmbientLightChanged:
    {
      if (m_Type == ActionType::AddAmbientLight)
      {
        SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      }
    }
    break;

    case ezGameObjectEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;
  }
}

void ezGameObjectDocumentAction::OnPreferenceChange(ezPreferences* pref)
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      SetChecked(pPreferences->GetShowGrid());
    }
    break;
  }
}

ezCameraSpeedSliderAction::ezCameraSpeedSliderAction(const ezActionContext& context, const char* szName, ActionType type)
    : ezSliderAction(context, szName)
{
  m_Type = type;
  m_pGameObjectDocument = const_cast<ezGameObjectDocument*>(static_cast<const ezGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezCameraSpeedSliderAction::OnPreferenceChange, this));

      SetRange(0, 24);
    }
    break;
  }

  UpdateState();
}

ezCameraSpeedSliderAction::~ezCameraSpeedSliderAction()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezCameraSpeedSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void ezCameraSpeedSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->SetCameraSpeed(value.Get<ezInt32>());
    }
    break;
  }
}

void ezCameraSpeedSliderAction::OnPreferenceChange(ezPreferences* pref)
{
  UpdateState();
}

void ezCameraSpeedSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(m_pGameObjectDocument);

      SetValue(pPreferences->GetCameraSpeed());
    }
    break;
  }
}
