#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <EditorPluginDLang/Actions/DLangActions.h>
#include <EditorPluginDLang/DLangAsset/DLangAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezDLangActions::s_hCategory;
ezActionDescriptorHandle ezDLangActions::s_hEditScript;


void ezDLangActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("DLangCategory");
  s_hEditScript = EZ_REGISTER_ACTION_1("DLang.Edit", ezActionScope::Document, "DLangs", "Edit Script", ezDLangAction, ezDLangAction::ActionType::EditScript);
}

void ezDLangActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hEditScript);
}

void ezDLangActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "DLangCategory";

  pMap->MapAction(s_hEditScript, szSubPath, 1.0f);
}

ezDLangAction::ezDLangAction(const ezActionContext& context, const char* szName, ezDLangAction::ActionType type, float fSimSpeed)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<ezDLangAssetDocument*>(static_cast<const ezDLangAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::EditScript:
      SetIconPath(":/GuiFoundation/Icons/vscode16.png");
      break;
  }
}


void ezDLangAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::EditScript:
      m_pDocument->EditScript();
      return;
  }
}
