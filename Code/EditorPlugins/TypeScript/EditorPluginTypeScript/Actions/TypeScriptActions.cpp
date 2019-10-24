#include <EditorPluginTypeScriptPCH.h>

#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezTypeScriptActions::s_hCategory;
ezActionDescriptorHandle ezTypeScriptActions::s_hEditScript;


void ezTypeScriptActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("TypeScriptCategory");
  s_hEditScript = EZ_REGISTER_ACTION_1("TypeScript.Edit", ezActionScope::Document, "TypeScripts", "Edit Script", ezTypeScriptAction, ezTypeScriptAction::ActionType::EditScript);
}

void ezTypeScriptActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hEditScript);
}

void ezTypeScriptActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "TypeScriptCategory";

  pMap->MapAction(s_hEditScript, szSubPath, 1.0f);
}

ezTypeScriptAction::ezTypeScriptAction(const ezActionContext& context, const char* szName, ezTypeScriptAction::ActionType type, float fSimSpeed)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<ezTypeScriptAssetDocument*>(static_cast<const ezTypeScriptAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::EditScript:
      SetIconPath(":/EditorPluginTypeScript/Icons/EditScript16.png");
      break;
  }
}


void ezTypeScriptAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::EditScript:
      m_pDocument->EditScript();
      return;
  }
}
