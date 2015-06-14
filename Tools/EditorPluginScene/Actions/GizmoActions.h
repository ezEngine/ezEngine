#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezSceneDocument;
enum class ActiveGizmo;

///
class EZ_EDITORPLUGINSCENE_DLL ezGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hGizmoCategory;
  static ezActionDescriptorHandle s_hNoGizmo;
  static ezActionDescriptorHandle s_hTranslateGizmo;
  static ezActionDescriptorHandle s_hRotateGizmo;
  static ezActionDescriptorHandle s_hScaleGizmo;

};

///
class EZ_EDITORPLUGINSCENE_DLL ezGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoAction);
public:

  ezGizmoAction(const ezActionContext& context, const char* szName, ActiveGizmo button);
  ~ezGizmoAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void UpdateState();
  void SceneEventHandler(const ezSceneDocument::SceneEvent& e);

  ezSceneDocument* m_pSceneDocument;
  ActiveGizmo m_ButtonType;
};