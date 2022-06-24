#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void OnLoadPlugin()
{
  ezParticleActions::RegisterActions();

  // Particle Effect
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("ParticleEffectAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("ParticleEffectAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezProjectActions::MapActions("ParticleEffectAssetMenuBar");
      ezDocumentActions::MapActions("ParticleEffectAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("ParticleEffectAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("ParticleEffectAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("ParticleEffectAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("ParticleEffectAssetToolBar", "");
      ezAssetActions::MapActions("ParticleEffectAssetToolBar", true);
      ezParticleActions::MapActions("ParticleEffectAssetToolBar", "");
    }

    // View Tool Bar
    {
      ezActionMapManager::RegisterActionMap("ParticleEffectAssetViewToolBar").IgnoreResult();
      ezViewActions::MapActions("ParticleEffectAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
      ezViewLightActions::MapActions("ParticleEffectAssetViewToolBar", "");
    }

    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezParticleEffectAssetDocument::PropertyMetaStateEventHandler);
  }
}

void OnUnloadPlugin()
{
  ezParticleActions::UnregisterActions();
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezParticleEffectAssetDocument::PropertyMetaStateEventHandler);
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
