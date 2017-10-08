#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

///
class EZ_EDITORPLUGINSCENE_DLL ezSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);
  static void MapPrefabActions(const char* szMapping, const char* szPath, float fPriority);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hSelectionCategory;
  static ezActionDescriptorHandle s_hShowInScenegraph;
  static ezActionDescriptorHandle s_hFocusOnSelection;
  static ezActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static ezActionDescriptorHandle s_hGroupSelectedItems;
  static ezActionDescriptorHandle s_hCreateEmptyChildObject;
  static ezActionDescriptorHandle s_hCreateEmptyObjectAtPosition;
  static ezActionDescriptorHandle s_hHideSelectedObjects;
  static ezActionDescriptorHandle s_hHideUnselectedObjects;
  static ezActionDescriptorHandle s_hShowHiddenObjects;
  static ezActionDescriptorHandle s_hPrefabMenu;
  static ezActionDescriptorHandle s_hCreatePrefab;
  static ezActionDescriptorHandle s_hRevertPrefab;
  static ezActionDescriptorHandle s_hUnlinkFromPrefab;
  static ezActionDescriptorHandle s_hOpenPrefabDocument;
  static ezActionDescriptorHandle s_hDuplicateSpecial;
  static ezActionDescriptorHandle s_hDeltaTransform;
  static ezActionDescriptorHandle s_hSnapCameraToObject;
  static ezActionDescriptorHandle s_hSnapObjectToCamera;
  static ezActionDescriptorHandle s_hMoveCameraHere;
  static ezActionDescriptorHandle s_hAttachToObject;
  static ezActionDescriptorHandle s_hDetachFromParent;
  static ezActionDescriptorHandle s_hConvertToEnginePrefab;
  static ezActionDescriptorHandle s_hConvertToEditorPrefab;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezSelectionAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectionAction, ezButtonAction);

public:

  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    GroupSelectedItems,
    CreateEmptyChildObject,
    CreateEmptyObjectAtPosition,
    HideSelectedObjects,
    HideUnselectedObjects,
    ShowHiddenObjects,
    CreatePrefab,
    RevertPrefab,
    UnlinkFromPrefab,
    OpenPrefabDocument,
    DuplicateSpecial,
    DeltaTransform,
    SnapCameraToObject,
    SnapObjectToCamera,
    MoveCameraHere,
    AttachToObject,
    DetachFromParent,
    ConvertToEnginePrefab,
    ConvertToEditorPrefab,
  };

  ezSelectionAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezSelectionAction();

  virtual void Execute(const ezVariant& value) override;

  void OpenPrefabDocument();

  void CreatePrefab();

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  void UpdateEnableState();

  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};




