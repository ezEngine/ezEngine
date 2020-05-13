#include <EditorPluginParticlePCH.h>

#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezParticleActions::s_hCategory;
ezActionDescriptorHandle ezParticleActions::s_hPauseEffect;
ezActionDescriptorHandle ezParticleActions::s_hRestartEffect;
ezActionDescriptorHandle ezParticleActions::s_hAutoRestart;
ezActionDescriptorHandle ezParticleActions::s_hSimulationSpeedMenu;
ezActionDescriptorHandle ezParticleActions::s_hSimulationSpeed[10];
ezActionDescriptorHandle ezParticleActions::s_hRenderVisualizers;


void ezParticleActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("ParticleCategory");
  s_hPauseEffect = EZ_REGISTER_ACTION_1("PFX.Pause", ezActionScope::Document, "Particles", "Pause", ezParticleAction,
    ezParticleAction::ActionType::PauseEffect);
  s_hRestartEffect = EZ_REGISTER_ACTION_1("PFX.Restart", ezActionScope::Document, "Particles", "F5", ezParticleAction,
    ezParticleAction::ActionType::RestartEffect);
  s_hAutoRestart = EZ_REGISTER_ACTION_1("PFX.AutoRestart", ezActionScope::Document, "Particles", "", ezParticleAction,
    ezParticleAction::ActionType::AutoRestart);

  s_hSimulationSpeedMenu = EZ_REGISTER_MENU_WITH_ICON("PFX.Speed.Menu", ":/EditorPluginParticle/Icons/Speed16.png");
  s_hSimulationSpeed[0] = EZ_REGISTER_ACTION_2("PFX.Speed.01", ezActionScope::Document, "Particles", "Ctrl+1", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = EZ_REGISTER_ACTION_2("PFX.Speed.025", ezActionScope::Document, "Particles", "Ctrl+2", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = EZ_REGISTER_ACTION_2("PFX.Speed.05", ezActionScope::Document, "Particles", "Ctrl+3", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = EZ_REGISTER_ACTION_2("PFX.Speed.1", ezActionScope::Document, "Particles", "Ctrl+4", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = EZ_REGISTER_ACTION_2("PFX.Speed.15", ezActionScope::Document, "Particles", "Ctrl+5", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = EZ_REGISTER_ACTION_2("PFX.Speed.2", ezActionScope::Document, "Particles", "Ctrl+6", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = EZ_REGISTER_ACTION_2("PFX.Speed.3", ezActionScope::Document, "Particles", "Ctrl+7", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = EZ_REGISTER_ACTION_2("PFX.Speed.4", ezActionScope::Document, "Particles", "Ctrl+8", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = EZ_REGISTER_ACTION_2("PFX.Speed.5", ezActionScope::Document, "Particles", "Ctrl+9", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = EZ_REGISTER_ACTION_2("PFX.Speed.10", ezActionScope::Document, "Particles", "Ctrl+0", ezParticleAction,
    ezParticleAction::ActionType::SimulationSpeed, 10.0f);
  s_hRenderVisualizers = EZ_REGISTER_ACTION_1("PFX.Render.Visualizers", ezActionScope::Document, "Particles", "V", ezParticleAction,
    ezParticleAction::ActionType::RenderVisualizers);
}

void ezParticleActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPauseEffect);
  ezActionManager::UnregisterAction(s_hRestartEffect);
  ezActionManager::UnregisterAction(s_hAutoRestart);
  ezActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  ezActionManager::UnregisterAction(s_hRenderVisualizers);

  for (int i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    ezActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void ezParticleActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "ParticleCategory";

  pMap->MapAction(s_hPauseEffect, szSubPath, 0.5f);
  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
  pMap->MapAction(s_hAutoRestart, szSubPath, 2.0f);

  pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

  ezStringBuilder sSubPath(szSubPath, "/PFX.Speed.Menu");

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hRenderVisualizers, szSubPath, 4.0f);
}

ezParticleAction::ezParticleAction(const ezActionContext& context, const char* szName, ezParticleAction::ActionType type, float fSimSpeed)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_fSimSpeed = fSimSpeed;

  m_pEffectDocument = const_cast<ezParticleEffectAssetDocument*>(static_cast<const ezParticleEffectAssetDocument*>(context.m_pDocument));
  m_pEffectDocument->m_Events.AddEventHandler(ezMakeDelegate(&ezParticleAction::EffectEventHandler, this));

  switch (m_Type)
  {
    case ActionType::PauseEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Pause16.png");
      break;

    case ActionType::RestartEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Restart16.png");
      break;

    case ActionType::AutoRestart:
      SetIconPath(":/EditorPluginParticle/Icons/Loop16.png");
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers16.png");
      SetChecked(m_pEffectDocument->GetRenderVisualizers());
      break;
  }

  UpdateState();
}


ezParticleAction::~ezParticleAction()
{
  m_pEffectDocument->m_Events.RemoveEventHandler(ezMakeDelegate(&ezParticleAction::EffectEventHandler, this));
}

void ezParticleAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PauseEffect:
      m_pEffectDocument->SetSimulationPaused(!m_pEffectDocument->GetSimulationPaused());
      return;

    case ActionType::RestartEffect:
      m_pEffectDocument->TriggerRestartEffect();
      return;

    case ActionType::AutoRestart:
      m_pEffectDocument->SetAutoRestart(!m_pEffectDocument->GetAutoRestart());
      return;

    case ActionType::SimulationSpeed:
      m_pEffectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::RenderVisualizers:
      m_pEffectDocument->SetRenderVisualizers(!m_pEffectDocument->GetRenderVisualizers());
      return;
  }
}

void ezParticleAction::EffectEventHandler(const ezParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case ezParticleEffectAssetEvent::AutoRestartChanged:
    case ezParticleEffectAssetEvent::SimulationSpeedChanged:
    case ezParticleEffectAssetEvent::RenderVisualizersChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void ezParticleAction::UpdateState()
{
  if (m_Type == ActionType::PauseEffect)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationPaused());
  }

  if (m_Type == ActionType::AutoRestart)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetAutoRestart());
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationSpeed() == m_fSimSpeed);
  }

  if (m_Type == ActionType::RenderVisualizers)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetRenderVisualizers());
  }
}
