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
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

void UpdateCollisionLayerDynamicEnumValues();
void UpdateWeightCategoryDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void ezDynamicActorComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezRagdollComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezCharacterControllerComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezRopeComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezClothSheetComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

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

  // component property meta states
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezDynamicActorComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezRagdollComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCharacterControllerComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezRopeComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezClothSheetComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezDynamicActorComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezRagdollComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCharacterControllerComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezRopeComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezClothSheetComponent_PropertyMetaStateEventHandler);

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

void UpdateWeightCategoryDynamicEnumValues()
{
  auto& cfe = ezDynamicEnum::GetDynamicEnum("PhysicsWeightCategoryWithDensity");
  auto& cfeNC = ezDynamicEnum::GetDynamicEnum("PhysicsWeightCategory");

  cfe.Clear();
  cfe.SetEditCommand("Jolt.Settings.Project", "WeightCategories");

  cfeNC.Clear();
  cfeNC.SetEditCommand("Jolt.Settings.Project", "WeightCategories");

  ezWeightCategoryConfig cfg;
  if (cfg.Load().Succeeded())
  {
    for (const auto it : cfg.m_Categories)
    {
      cfe.SetValueAndName(it.key, it.value.m_sName.GetView());
      cfeNC.SetValueAndName(it.key, it.value.m_sName.GetView());
    }
  }

  cfeNC.SetValueAndName(0, "<Default>");
  cfeNC.SetValueAndName(1, "<Custom Mass>");

  cfe.SetValueAndName(0, "<Default>");
  cfe.SetValueAndName(1, "<Custom Mass>");
  cfe.SetValueAndName(2, "<Custom Density>");
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectSaveState)
  {
    ezQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezQtJoltProjectSettingsDlg::EnsureConfigFileExists();
    UpdateCollisionLayerDynamicEnumValues();
    UpdateWeightCategoryDynamicEnumValues();
  }
}


//////////////////////////////////////////////////////////////////////////

void ezJoltWeightComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  auto& props = *e.m_pPropertyStates;

  const ezInt32 iCategory = e.m_pObject->GetTypeAccessor().GetValue("WeightCategory").ConvertTo<ezInt32>();

  if (iCategory == 0) // Default
  {
    props["WeightScale"].m_Visibility = ezPropertyUiState::Invisible;
    props["Mass"].m_Visibility = ezPropertyUiState::Invisible;
    props["Density"].m_Visibility = ezPropertyUiState::Invisible;
  }
  else if (iCategory == 1) // Custom Mass
  {
    props["WeightScale"].m_Visibility = ezPropertyUiState::Invisible;
    props["Density"].m_Visibility = ezPropertyUiState::Invisible;
  }
  else if (iCategory == 2) // Custom Density
  {
    props["WeightScale"].m_Visibility = ezPropertyUiState::Invisible;
    props["Mass"].m_Visibility = ezPropertyUiState::Invisible;
  }
  else
  {
    props["Density"].m_Visibility = ezPropertyUiState::Invisible;
    props["Mass"].m_Visibility = ezPropertyUiState::Invisible;
  }
}

void ezDynamicActorComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezJoltDynamicActorComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  ezJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void ezRagdollComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezJoltRagdollComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  ezJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void ezCharacterControllerComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezJoltCharacterControllerComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  ezJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void ezRopeComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezJoltRopeComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  ezJoltWeightComponent_PropertyMetaStateEventHandler(e);
}

void ezClothSheetComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezJoltClothSheetComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  ezJoltWeightComponent_PropertyMetaStateEventHandler(e);
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

//////////////////////////////////////////////////////////////////////////

class ezJoltDynamicActorComponentPatch_5_6 : public ezGraphPatch
{
public:
  ezJoltDynamicActorComponentPatch_5_6()
    : ezGraphPatch("ezJoltDynamicActorComponent", 6)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto pPropMass = pNode->FindProperty("Mass");

    float fMass = 0.0f;

    if (pPropMass && pPropMass->m_Value.IsNumber())
    {
      fMass = pPropMass->m_Value.ConvertTo<float>();
    }

    if (fMass != 0.0f)
    {
      pNode->AddProperty("WeightCategory", static_cast<ezUInt8>(1)); // Custom Mass
    }
    else
    {
      pNode->AddProperty("WeightCategory", static_cast<ezUInt8>(2)); // Custom Density
    }
  }
};

ezJoltDynamicActorComponentPatch_5_6 g_ezJoltDynamicActorComponentPatch_5_6;
