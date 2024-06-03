#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar", "AssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar", "AssetToolbar");
      ezCommonAssetActions::MapToolbarActions("JoltCollisionMeshAssetToolBar", ezCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezJoltActions::RegisterActions();
      ezJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  ezJoltActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = ezDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  ezCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
  {
    return;
  }

  // add all names and values that are valid (non-empty)
  for (ezInt32 i = 0; i < 32; ++i)
  {
    if (!cfg.GetGroupName(i).IsEmpty())
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectSaveState)
  {
    ezQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezJoltRopeComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezJoltRopeComponentPatch_1_2()
    : ezGraphPatch("ezJoltRopeComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

ezJoltRopeComponentPatch_1_2 g_ezJoltRopeComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class ezJoltHitboxComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezJoltHitboxComponentPatch_1_2()
    : ezGraphPatch("ezJoltBoneColliderComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezJoltHitboxComponent");
  }
};

ezJoltHitboxComponentPatch_1_2 g_ezJoltHitboxComponentPatch_1_2;
