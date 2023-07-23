#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptActions.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezVisualScriptActions::s_hCategory;
ezActionDescriptorHandle ezVisualScriptActions::s_hPickDebugTarget;

void ezVisualScriptActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("VisualScriptCategory");
  s_hPickDebugTarget = EZ_REGISTER_ACTION_1(
    "VisScript.PickDebugTarget", ezActionScope::Window, "Visual Script", "", ezVisualScriptAction, ezVisualScriptAction::ActionType::PickDebugTarget);
}

void ezVisualScriptActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hPickDebugTarget);
}

void ezVisualScriptActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const char* szSubPath = "VisualScriptCategory";

  pMap->MapAction(s_hPickDebugTarget, szSubPath, 1.0f);
}

ezVisualScriptAction::ezVisualScriptAction(const ezActionContext& context, const char* szName, ezVisualScriptAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::PickDebugTarget:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
  }
}

ezVisualScriptAction::~ezVisualScriptAction() = default;

void ezVisualScriptAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PickDebugTarget:
      static_cast<ezQtVisualScriptAssetDocumentWindow*>(GetContext().m_pWindow)->PickDebugTarget();
      return;
  }
}
