#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class EZ_EDITORPLUGINSCENE_DLL ezSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);
  static void MapPrefabActions(ezStringView sMapping, float fPriority);
  static void MapContextMenuActions(ezStringView sMapping);
  static void MapViewContextMenuActions(ezStringView sMapping);

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
  static ezActionDescriptorHandle s_hSnapObjectToCamera;
  static ezActionDescriptorHandle s_hAttachToObject;
  static ezActionDescriptorHandle s_hDetachFromParent;
  static ezActionDescriptorHandle s_hConvertToEnginePrefab;
  static ezActionDescriptorHandle s_hConvertToEditorPrefab;
  static ezActionDescriptorHandle s_hCopyReference;
  static ezActionDescriptorHandle s_hSelectParent;
  static ezActionDescriptorHandle s_hSetActiveParent;
  static ezActionDescriptorHandle s_hClearActiveParent;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezSelectionAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectionAction, ezButtonAction);

public:
  enum class ActionType
  {
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
    ConvertToEnginePrefab,
    ConvertToEditorPrefab,

    DuplicateSpecial,
    DeltaTransform,
    SnapObjectToCamera,
    AttachToObject,
    DetachFromParent,
    CopyReference,
    SelectParent,

    SetActiveParent,
    ClearActiveParent,
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
