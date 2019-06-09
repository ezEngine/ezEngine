#include <EditorPluginRecastPCH.h>

#include <EditorPluginRecast/Actions/RecastActions.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecastAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRecastAction::ezRecastAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, nullptr)
{
  m_ButtonType = button;
}

ezRecastAction::~ezRecastAction() = default;

void ezRecastAction::Execute(const ezVariant& value)
{
  auto pLongOp = EZ_DEFAULT_NEW(ezLongOpProxy_BuildNavMesh);

  ezStringBuilder sOutputFile = GetContext().m_pDocument->GetDocumentPath();
  sOutputFile.RemoveFileExtension();
  sOutputFile.Append("_data/Scene.ezRecastNavMesh");
  pLongOp->m_sOutputPath = sOutputFile;

  ezLongOpManager::GetSingleton()->AddLongOperation(std::move(pLongOp), GetContext().m_pDocument->GetGuid());
}

//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezRecastActions::s_hRecastMenu;
ezActionDescriptorHandle ezRecastActions::s_hNavMeshCategory;
ezActionDescriptorHandle ezRecastActions::s_hGenerateNavMesh;

void ezRecastActions::RegisterActions()
{
  s_hRecastMenu = EZ_REGISTER_MENU("Menu.Recast");
  s_hNavMeshCategory = EZ_REGISTER_CATEGORY("NavMeshCategory");
  s_hGenerateNavMesh = EZ_REGISTER_ACTION_1(
    "NavMesh.Generate", ezActionScope::Document, "Recast - NavMesh", "", ezRecastAction, ezRecastAction::ButtonType::GenerateNavMesh);
}

void ezRecastActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRecastMenu);
  ezActionManager::UnregisterAction(s_hNavMeshCategory);
  ezActionManager::UnregisterAction(s_hGenerateNavMesh);
}

void ezRecastActions::MapActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hRecastMenu, "", 10.0f);

  pMap->MapAction(s_hNavMeshCategory, "Menu.Recast", 1.0f);
  pMap->MapAction(s_hGenerateNavMesh, "Menu.Recast/NavMeshCategory", 1.0f);
}
