#include <PCH.h>

#include <EditorPluginProceduralPlacement/Actions/ProceduralPlacementActions.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezProceduralPlacementActions::s_hCategoryProceduralPlacement;
ezActionDescriptorHandle ezProceduralPlacementActions::s_hDumpAST;
ezActionDescriptorHandle ezProceduralPlacementActions::s_hDumpDisassembly;

void ezProceduralPlacementActions::RegisterActions()
{
  s_hCategoryProceduralPlacement = EZ_REGISTER_CATEGORY("ProceduralPlacement");
  s_hDumpAST = EZ_REGISTER_ACTION_1("ProceduralPlacement.DumpAST", ezActionScope::Document, "Procedural Placement", "",
                                    ezProceduralPlacementAction, ezProceduralPlacementAction::ActionType::DumpAST);
  s_hDumpDisassembly = EZ_REGISTER_ACTION_1("ProceduralPlacement.DumpDisassembly", ezActionScope::Document, "Procedural Placement", "",
                                            ezProceduralPlacementAction, ezProceduralPlacementAction::ActionType::DumpDisassembly);
}

void ezProceduralPlacementActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryProceduralPlacement);
  ezActionManager::UnregisterAction(s_hDumpAST);
  ezActionManager::UnregisterAction(s_hDumpDisassembly);
}

void ezProceduralPlacementActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("ProceduralPlacementAssetMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  pMap->MapAction(s_hCategoryProceduralPlacement, "Menu.Tools", 9.0f);
  pMap->MapAction(s_hDumpAST, "Menu.Tools", 10.0f);
  pMap->MapAction(s_hDumpDisassembly, "Menu.Tools", 11.0f);

  pMap = ezActionMapManager::GetActionMap("ProceduralPlacementAssetToolBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mmapping the actions failed!");

  pMap->MapAction(s_hCategoryProceduralPlacement, "", 12.0f);
  pMap->MapAction(s_hDumpAST, "ProceduralPlacement", 0.0f);
  pMap->MapAction(s_hDumpDisassembly, "ProceduralPlacement", 0.0f);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProceduralPlacementAction::ezProceduralPlacementAction(const ezActionContext& context, const char* szName, ActionType type)
    : ezButtonAction(context, szName, false, "")
    , m_Type(type)
{
}

ezProceduralPlacementAction::~ezProceduralPlacementAction() {}

void ezProceduralPlacementAction::Execute(const ezVariant& value)
{
  if (auto pAssetDocument = ezDynamicCast<ezProceduralPlacementAssetDocument*>(GetContext().m_pDocument))
  {
    pAssetDocument->DumpSelectedOutput(m_Type == ActionType::DumpAST, m_Type == ActionType::DumpDisassembly);
  }
}
