#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezGameObjectDocument;

///
class EZ_EDITORFRAMEWORK_DLL ezGameObjectSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hSelectionCategory;
  static ezActionDescriptorHandle s_hShowInScenegraph;
  static ezActionDescriptorHandle s_hFocusOnSelection;
  static ezActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static ezActionDescriptorHandle s_hSnapCameraToObject;
  static ezActionDescriptorHandle s_hMoveCameraHere;
  static ezActionDescriptorHandle s_hCreateEmptyGameObjectHere;
};

///
class EZ_EDITORFRAMEWORK_DLL ezGameObjectSelectionAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectSelectionAction, ezButtonAction);

public:

  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    SnapCameraToObject,
    MoveCameraHere,
    CreateGameObjectHere,
  };

  ezGameObjectSelectionAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezGameObjectSelectionAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  void UpdateEnableState();

  ezGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};




