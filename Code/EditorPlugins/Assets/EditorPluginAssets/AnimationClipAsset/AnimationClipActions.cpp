#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationClipAsset/AnimationClipActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezAnimationClipActions::s_hCategory;
ezActionDescriptorHandle ezAnimationClipActions::s_hPauseEffect;
ezActionDescriptorHandle ezAnimationClipActions::s_hRestartEffect;
ezActionDescriptorHandle ezAnimationClipActions::s_hAutoRestart;
ezActionDescriptorHandle ezAnimationClipActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezAnimationClipActions::s_hSimulationSpeed[10];


void ezAnimationClipActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("AnimationClipCategory");
  s_hPauseEffect = EZ_REGISTER_ACTION_1("AnimClip.Pause", ezActionScope::Document, "Animations", "Pause", ezAnimationClipAction, ezAnimationClipAction::ActionType::Pause);
  s_hRestartEffect = EZ_REGISTER_ACTION_1("AnimClip.Restart", ezActionScope::Document, "Animations", "F5", ezAnimationClipAction, ezAnimationClipAction::ActionType::Restart);
  s_hAutoRestart = EZ_REGISTER_ACTION_1("AnimClip.Loop", ezActionScope::Document, "Animations", "", ezAnimationClipAction, ezAnimationClipAction::ActionType::Loop);

  s_hSimulationSpeedMenu = EZ_REGISTER_MENU_WITH_ICON("AnimClip.Speed.Menu", ":/EditorPluginParticle/Icons/Speed16.png");
  s_hSimulationSpeed[0] = EZ_REGISTER_ACTION_2("AnimClip.Speed.01", ezActionScope::Document, "Animations", "Ctrl+1", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = EZ_REGISTER_ACTION_2("AnimClip.Speed.025", ezActionScope::Document, "Animations", "Ctrl+2", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = EZ_REGISTER_ACTION_2("AnimClip.Speed.05", ezActionScope::Document, "Animations", "Ctrl+3", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = EZ_REGISTER_ACTION_2("AnimClip.Speed.1", ezActionScope::Document, "Animations", "Ctrl+4", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = EZ_REGISTER_ACTION_2("AnimClip.Speed.15", ezActionScope::Document, "Animations", "Ctrl+5", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = EZ_REGISTER_ACTION_2("AnimClip.Speed.2", ezActionScope::Document, "Animations", "Ctrl+6", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = EZ_REGISTER_ACTION_2("AnimClip.Speed.3", ezActionScope::Document, "Animations", "Ctrl+7", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = EZ_REGISTER_ACTION_2("AnimClip.Speed.4", ezActionScope::Document, "Animations", "Ctrl+8", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = EZ_REGISTER_ACTION_2("AnimClip.Speed.5", ezActionScope::Document, "Animations", "Ctrl+9", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("AnimClip.Speed.10", ezActionScope::Document, "Animations", "Ctrl+0", ezAnimationClipAction, ezAnimationClipAction::ActionType::SimulationSpeed, 10.0f);
}

void ezAnimationClipActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPauseEffect);
  ezActionManager::UnregisterAction(s_hRestartEffect);
  ezActionManager::UnregisterAction(s_hAutoRestart);
  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void ezAnimationClipActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "AnimationClipCategory";

  pMap->MapAction(s_hPauseEffect, szSubPath, 0.5f);
  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
  pMap->MapAction(s_hAutoRestart, szSubPath, 2.0f);

  pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

  ezStringBuilder sSubPath(szSubPath, "/AnimClip.Speed.Menu");

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);
}

ezAnimationClipAction::ezAnimationClipAction(const ezActionContext& context, const char* szName, ezAnimationClipAction::ActionType type, float fSimSpeed)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pAnimDocument = const_cast<ezAnimationClipAssetDocument*>(static_cast<const ezAnimationClipAssetDocument*>(context.m_pDocument));
  m_pAnimDocument->m_Events.AddEventHandler(ezMakeDelegate(&ezAnimationClipAction::AnimEventHandler, this));

  switch (m_Type)
  {
    case ActionType::Pause:
      SetIconPath(":/EditorPluginParticle/Icons/Pause16.png");
      break;

    case ActionType::Restart:
      SetIconPath(":/EditorPluginParticle/Icons/Restart16.png");
      break;

    case ActionType::Loop:
      SetIconPath(":/EditorPluginParticle/Icons/Loop16.png");
      break;

    default:
      break;
  }

  UpdateState();
}


ezAnimationClipAction::~ezAnimationClipAction()
{
  m_pAnimDocument->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimationClipAction::AnimEventHandler, this));
}

void ezAnimationClipAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::Pause:
      m_pAnimDocument->SetSimulationPaused(!m_pAnimDocument->GetSimulationPaused());
      return;

    case ActionType::Restart:
      m_pAnimDocument->TriggerRestart();
      return;

    case ActionType::Loop:
      m_pAnimDocument->SetLoop(!m_pAnimDocument->GetLoop());
      return;

    case ActionType::SimulationSpeed:
      m_pAnimDocument->SetSimulationSpeed(m_fSimSpeed);
      return;
  }
}

void ezAnimationClipAction::AnimEventHandler(const ezAnimationClipAssetEvent& e)
{
  switch (e.m_Type)
  {
    case ezAnimationClipAssetEvent::LoopChanged:
    case ezAnimationClipAssetEvent::SimulationSpeedChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void ezAnimationClipAction::UpdateState()
{
  if (m_Type == ActionType::Pause)
  {
    SetCheckable(true);
    SetChecked(m_pAnimDocument->GetSimulationPaused());
  }

  if (m_Type == ActionType::Loop)
  {
    SetCheckable(true);
    SetChecked(m_pAnimDocument->GetLoop());
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pAnimDocument->GetSimulationSpeed() == m_fSimSpeed);
  }
}
