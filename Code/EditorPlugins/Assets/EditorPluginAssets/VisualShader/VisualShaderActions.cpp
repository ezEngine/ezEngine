#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezVisualShaderActions::s_hVisualShaderCategory;
ezActionDescriptorHandle ezVisualShaderActions::s_hCleanGraph;

void ezVisualShaderActions::RegisterActions()
{
  s_hVisualShaderCategory = EZ_REGISTER_CATEGORY("VisualShaderCategory");
  s_hCleanGraph = EZ_REGISTER_ACTION_0("VisualShader.CleanGraph", ezActionScope::Document, "Visual Shader", "", ezVisualShaderAction);
}

void ezVisualShaderActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hVisualShaderCategory);
  ezActionManager::UnregisterAction(s_hCleanGraph);
}

void ezVisualShaderActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  // Use a high sort key to place Visual Shader actions on the far right of the toolbar
  pMap->MapAction(s_hVisualShaderCategory, "", 1000.0f);
  pMap->MapAction(s_hCleanGraph, "VisualShaderCategory", 1.0f);
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualShaderAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualShaderAction::ezVisualShaderAction(const ezActionContext& context, const char* szName)
  : ezButtonAction(context, szName, false, "")
{
  SetIconPath(":/EditorPluginAssets/Cleanup.svg");

  // Register to listen for property changes on the document
  m_Context.m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezVisualShaderAction::PropertyEventHandler, this));

  // Initialize visibility based on current shader mode
  ezMaterialAssetDocument* pMaterial = static_cast<ezMaterialAssetDocument*>(m_Context.m_pDocument);
  const bool bCustom = pMaterial->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() == ezMaterialShaderMode::Custom;
  SetVisible(bCustom, false);
}

ezVisualShaderAction::~ezVisualShaderAction()
{
  m_Context.m_pDocument->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualShaderAction::PropertyEventHandler, this));
}

void ezVisualShaderAction::Execute(const ezVariant& value)
{
  ezMaterialAssetDocument* pMaterial = static_cast<ezMaterialAssetDocument*>(m_Context.m_pDocument);
  pMaterial->RemoveDisconnectedNodes();
}

void ezVisualShaderAction::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  ezMaterialAssetDocument* pMaterial = static_cast<ezMaterialAssetDocument*>(m_Context.m_pDocument);

  // Only react to ShaderMode changes on the material property object
  if (e.m_pObject == pMaterial->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    const bool bCustom = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() == ezMaterialShaderMode::Custom;
    SetVisible(bCustom);
  }
}
