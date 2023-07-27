#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

ezActionDescriptorHandle ezSceneGizmoActions::s_hGreyBoxingGizmo;

void ezSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", ezActionScope::Document, "Gizmo", "B", ezGizmoAction, ezGetStaticRTTI<ezGreyBoxEditTool>());
}

void ezSceneGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void ezSceneGizmoActions::MapMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGreyBoxingGizmo, "G.Gizmos", 5.0f);
}

void ezSceneGizmoActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const ezStringView sSubPath("GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
