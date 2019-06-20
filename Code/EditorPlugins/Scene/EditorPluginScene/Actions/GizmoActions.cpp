#include <EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezSceneGizmoActions::s_hGreyBoxingGizmo;

void ezSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", ezActionScope::Document, "Gizmo", "B", ezGizmoAction,
                                            ezGetStaticRTTI<ezGreyBoxEditTool>());
}

void ezSceneGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void ezSceneGizmoActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Menu");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}

void ezSceneGizmoActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
