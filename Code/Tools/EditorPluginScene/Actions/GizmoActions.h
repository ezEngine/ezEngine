#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/////
class EZ_EDITORPLUGINSCENE_DLL ezSceneGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping, const char* szPath);
  static void MapToolbarActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hGreyBoxingGizmo;
};
