#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/////
class EZ_EDITORPLUGINSCENE_DLL ezSceneGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(ezStringView sMapping);
  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hGreyBoxingGizmo;
};
