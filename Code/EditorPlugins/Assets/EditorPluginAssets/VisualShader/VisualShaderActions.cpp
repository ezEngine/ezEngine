#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezVisualShaderActions::s_hCleanGraph;

void ezVisualShaderActions::RegisterActions()
{
  s_hCleanGraph = EZ_REGISTER_ACTION_0("VisualShader.CleanGraph", ezActionScope::Document, "Visual Shader", "", ezVisualShaderAction);
}

void ezVisualShaderActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCleanGraph);
}

void ezVisualShaderActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCleanGraph, "", 30.0f);
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualShaderAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualShaderAction::ezVisualShaderAction(const ezActionContext& context, const char* szName)
  : ezButtonAction(context, szName, false, "")
{
  SetIconPath(":/EditorPluginAssets/Cleanup.svg");
}

ezVisualShaderAction::~ezVisualShaderAction() = default;

void ezVisualShaderAction::Execute(const ezVariant& value)
{
  ezMaterialAssetDocument* pMaterial = (ezMaterialAssetDocument*)(m_Context.m_pDocument);

  pMaterial->RemoveDisconnectedNodes();
}
