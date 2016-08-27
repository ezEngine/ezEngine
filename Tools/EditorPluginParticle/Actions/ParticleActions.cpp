#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezParticleActions::s_hCategory;
ezActionDescriptorHandle ezParticleActions::s_hRestartEffect;

void ezParticleActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("ParticleCategory");
  s_hRestartEffect = EZ_REGISTER_ACTION_1("PFX.Restart", ezActionScope::Document, "Particles - Restart", "F5", ezParticleAction, ezParticleAction::ActionType::RestartEffect);
}

void ezParticleActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hRestartEffect);
}

void ezParticleActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "ParticleCategory";

  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
}

ezParticleAction::ezParticleAction(const ezActionContext& context, const char* szName, ezParticleAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pEffectDocument = const_cast<ezParticleEffectAssetDocument*>(static_cast<const ezParticleEffectAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
  case ActionType::RestartEffect:
    //SetIconPath(":/EditorPluginScene/Icons/Scenegraph16.png"); /// \todo Icon
    break;
  }
}


ezParticleAction::~ezParticleAction()
{
}

void ezParticleAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::RestartEffect:
    m_pEffectDocument->TriggerRestartEffect();
    return;
  }
}

