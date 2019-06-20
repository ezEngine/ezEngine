#include <EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezProcGenActions::s_hCategory;
ezActionDescriptorHandle ezProcGenActions::s_hDumpAST;
ezActionDescriptorHandle ezProcGenActions::s_hDumpDisassembly;

void ezProcGenActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("ProcGen");
  s_hDumpAST = EZ_REGISTER_ACTION_1(
    "ProcGen.DumpAST", ezActionScope::Document, "ProcGen Graph", "", ezProcGenAction, ezProcGenAction::ActionType::DumpAST);
  s_hDumpDisassembly = EZ_REGISTER_ACTION_1("ProcGen.DumpDisassembly", ezActionScope::Document, "ProcGen Graph", "", ezProcGenAction,
    ezProcGenAction::ActionType::DumpDisassembly);
}

void ezProcGenActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hDumpAST);
  ezActionManager::UnregisterAction(s_hDumpDisassembly);
}

void ezProcGenActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("ProcGenAssetMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  pMap->MapAction(s_hCategory, "Menu.Tools", 9.0f);
  pMap->MapAction(s_hDumpAST, "Menu.Tools", 10.0f);
  pMap->MapAction(s_hDumpDisassembly, "Menu.Tools", 11.0f);

  pMap = ezActionMapManager::GetActionMap("ProcGenAssetToolBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  pMap->MapAction(s_hCategory, "", 12.0f);
  pMap->MapAction(s_hDumpAST, "ProcGen", 0.0f);
  pMap->MapAction(s_hDumpDisassembly, "ProcGen", 0.0f);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProcGenAction::ezProcGenAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
  , m_Type(type)
{
}

ezProcGenAction::~ezProcGenAction() {}

void ezProcGenAction::Execute(const ezVariant& value)
{
  if (auto pAssetDocument = ezDynamicCast<ezProcGenGraphAssetDocument*>(GetContext().m_pDocument))
  {
    pAssetDocument->DumpSelectedOutput(m_Type == ActionType::DumpAST, m_Type == ActionType::DumpDisassembly);
  }
}
