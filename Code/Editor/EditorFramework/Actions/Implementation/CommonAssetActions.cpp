#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CommonAssetActions.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCommonAssetAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezCommonAssetActions::s_hCategory;
ezActionDescriptorHandle ezCommonAssetActions::s_hPause;
ezActionDescriptorHandle ezCommonAssetActions::s_hRestart;
ezActionDescriptorHandle ezCommonAssetActions::s_hLoop;
ezActionDescriptorHandle ezCommonAssetActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezCommonAssetActions::s_hSimulationSpeed[10];
ezActionDescriptorHandle ezCommonAssetActions::s_hGrid;
ezActionDescriptorHandle ezCommonAssetActions::s_hVisualizers;


void ezCommonAssetActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("CommonAssetCategory");
  s_hPause = EZ_REGISTER_ACTION_1("Common.Pause", ezActionScope::Document, "Animations", "Pause", ezCommonAssetAction, ezCommonAssetAction::ActionType::Pause);
  s_hRestart = EZ_REGISTER_ACTION_1("Common.Restart", ezActionScope::Document, "Animations", "F5", ezCommonAssetAction, ezCommonAssetAction::ActionType::Restart);
  s_hLoop = EZ_REGISTER_ACTION_1("Common.Loop", ezActionScope::Document, "Animations", "", ezCommonAssetAction, ezCommonAssetAction::ActionType::Loop);
  s_hGrid = EZ_REGISTER_ACTION_1("Common.Grid", ezActionScope::Document, "Misc", "G", ezCommonAssetAction, ezCommonAssetAction::ActionType::Grid);
  s_hVisualizers = EZ_REGISTER_ACTION_1("Common.Visualizers", ezActionScope::Document, "Misc", "V", ezCommonAssetAction, ezCommonAssetAction::ActionType::Visualizers);

  s_hSimulationSpeedMenu = EZ_REGISTER_MENU_WITH_ICON("Common.Speed.Menu", ":/EditorPluginParticle/Icons/Speed16.png");
  s_hSimulationSpeed[0] = EZ_REGISTER_ACTION_2("Common.Speed.01", ezActionScope::Document, "Animations", "Ctrl+1", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = EZ_REGISTER_ACTION_2("Common.Speed.025", ezActionScope::Document, "Animations", "Ctrl+2", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = EZ_REGISTER_ACTION_2("Common.Speed.05", ezActionScope::Document, "Animations", "Ctrl+3", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = EZ_REGISTER_ACTION_2("Common.Speed.1", ezActionScope::Document, "Animations", "Ctrl+4", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = EZ_REGISTER_ACTION_2("Common.Speed.15", ezActionScope::Document, "Animations", "Ctrl+5", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = EZ_REGISTER_ACTION_2("Common.Speed.2", ezActionScope::Document, "Animations", "Ctrl+6", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = EZ_REGISTER_ACTION_2("Common.Speed.3", ezActionScope::Document, "Animations", "Ctrl+7", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = EZ_REGISTER_ACTION_2("Common.Speed.4", ezActionScope::Document, "Animations", "Ctrl+8", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = EZ_REGISTER_ACTION_2("Common.Speed.5", ezActionScope::Document, "Animations", "Ctrl+9", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("Common.Speed.10", ezActionScope::Document, "Animations", "Ctrl+0", ezCommonAssetAction, ezCommonAssetAction::ActionType::SimulationSpeed, 10.0f);
}

void ezCommonAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPause);
  ezActionManager::UnregisterAction(s_hRestart);
  ezActionManager::UnregisterAction(s_hLoop);
  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  ezActionManager::UnregisterAction(s_hGrid);
  ezActionManager::UnregisterAction(s_hVisualizers);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void ezCommonAssetActions::MapActions(const char* szMapping, const char* szPath, ezUInt32 stateMask)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "CommonAssetCategory";

  if (stateMask & ezCommonAssetUiState::Pause)
  {
    pMap->MapAction(s_hPause, szSubPath, 0.5f);
  }

  if (stateMask & ezCommonAssetUiState::Restart)
  {
    pMap->MapAction(s_hRestart, szSubPath, 1.0f);
  }

  if (stateMask & ezCommonAssetUiState::Loop)
  {
    pMap->MapAction(s_hLoop, szSubPath, 2.0f);
  }

  if (stateMask & ezCommonAssetUiState::SimulationSpeed)
  {
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

    ezStringBuilder sSubPath(szSubPath, "/Common.Speed.Menu");

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    {
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);
    }
  }

  if (stateMask & ezCommonAssetUiState::Grid)
  {
    pMap->MapAction(s_hGrid, szSubPath, 4.0f);
  }

  if (stateMask & ezCommonAssetUiState::Visualizers)
  {
    pMap->MapAction(s_hVisualizers, szSubPath, 5.0f);
  }
}

ezCommonAssetAction::ezCommonAssetAction(const ezActionContext& context, const char* szName, ezCommonAssetAction::ActionType type, float fSimSpeed)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pAssetDocument = const_cast<ezAssetDocument*>(static_cast<const ezAssetDocument*>(context.m_pDocument));
  m_pAssetDocument->m_CommonAssetUiChangeEvent.AddEventHandler(ezMakeDelegate(&ezCommonAssetAction::CommonUiEventHandler, this));

  switch (m_Type)
  {
    case ActionType::Pause:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Pause16.png");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Pause) != 0.0f);
      break;

    case ActionType::Restart:
      SetIconPath(":/EditorPluginParticle/Icons/Restart16.png");
      break;

    case ActionType::Loop:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Loop16.png");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Loop) != 0.0f);
      break;

    case ActionType::Grid:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid16.png");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Grid) != 0.0f);
      break;

    case ActionType::Visualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers16.png");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Visualizers) != 0.0f);
      break;

    default:
      break;
  }

  UpdateState();
}


ezCommonAssetAction::~ezCommonAssetAction()
{
  m_pAssetDocument->m_CommonAssetUiChangeEvent.RemoveEventHandler(ezMakeDelegate(&ezCommonAssetAction::CommonUiEventHandler, this));
}

void ezCommonAssetAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::Pause:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::Pause, m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Pause) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Restart:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::Restart, 1.0f);
      return;

    case ActionType::Loop:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::Loop, m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Loop) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::SimulationSpeed:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::SimulationSpeed, m_fSimSpeed);
      return;

    case ActionType::Grid:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::Grid, m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Grid) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Visualizers:
      m_pAssetDocument->SetCommonAssetUiState(ezCommonAssetUiState::Visualizers, m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Visualizers) == 0.0f ? 1.0f : 0.0f);
      return;
  }
}

void ezCommonAssetAction::CommonUiEventHandler(const ezCommonAssetUiState& e)
{
  if (e.m_State == ezCommonAssetUiState::Loop || e.m_State == ezCommonAssetUiState::SimulationSpeed || e.m_State == ezCommonAssetUiState::Grid || e.m_State == ezCommonAssetUiState::Visualizers)
  {
    UpdateState();
  }
}

void ezCommonAssetAction::UpdateState()
{
  if (m_Type == ActionType::Pause)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Pause) != 0.0f);
  }

  if (m_Type == ActionType::Loop)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Loop) != 0.0f);
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::SimulationSpeed) == m_fSimSpeed);
  }

  if (m_Type == ActionType::Grid)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Grid) != 0.0f);
  }

  if (m_Type == ActionType::Visualizers)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(ezCommonAssetUiState::Visualizers) != 0.0f);
  }
}
