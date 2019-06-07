#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezPreferences;

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();
  static void MapToolbarActions();
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hSceneCategory;
  static ezActionDescriptorHandle s_hSceneUtilsMenu;
  static ezActionDescriptorHandle s_hExportScene;
  static ezActionDescriptorHandle s_hRunScene;
  static ezActionDescriptorHandle s_hGameModeSimulate;
  static ezActionDescriptorHandle s_hGameModePlay;
  static ezActionDescriptorHandle s_hGameModePlayFromHere;
  static ezActionDescriptorHandle s_hGameModeStop;
  static ezActionDescriptorHandle s_hUtilExportSceneToOBJ;
  static ezActionDescriptorHandle s_hKeepSimulationChanges;
  static ezActionDescriptorHandle s_hCreateThumbnail;
  static ezActionDescriptorHandle s_hFavouriteCamsMenu;
  static ezActionDescriptorHandle s_hStoreEditorCamera[10];
  static ezActionDescriptorHandle s_hRestoreEditorCamera[10];
  static ezActionDescriptorHandle s_hJumpToCamera[10];
  static ezActionDescriptorHandle s_hCreateLevelCamera[10];
};

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneAction, ezButtonAction);

public:
  enum class ActionType
  {
    ExportScene,
    RunScene,
    StartGameModeSimulate,
    StartGameModePlay,
    StartGameModePlayFromHere,
    StopGameMode,
    ExportSceneToOBJ,
    KeepSimulationChanges,
    CreateThumbnail,

    StoreEditorCamera0,
    StoreEditorCamera1,
    StoreEditorCamera2,
    StoreEditorCamera3,
    StoreEditorCamera4,
    StoreEditorCamera5,
    StoreEditorCamera6,
    StoreEditorCamera7,
    StoreEditorCamera8,
    StoreEditorCamera9,

    RestoreEditorCamera0,
    RestoreEditorCamera1,
    RestoreEditorCamera2,
    RestoreEditorCamera3,
    RestoreEditorCamera4,
    RestoreEditorCamera5,
    RestoreEditorCamera6,
    RestoreEditorCamera7,
    RestoreEditorCamera8,
    RestoreEditorCamera9,

    JumpToCamera0,
    JumpToCamera1,
    JumpToCamera2,
    JumpToCamera3,
    JumpToCamera4,
    JumpToCamera5,
    JumpToCamera6,
    JumpToCamera7,
    JumpToCamera8,
    JumpToCamera9,

    CreateLevelCamera0,
    CreateLevelCamera1,
    CreateLevelCamera2,
    CreateLevelCamera3,
    CreateLevelCamera4,
    CreateLevelCamera5,
    CreateLevelCamera6,
    CreateLevelCamera7,
    CreateLevelCamera8,
    CreateLevelCamera9,
  };

  ezSceneAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezSceneAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SceneEventHandler(const ezGameObjectEvent& e);
  void UpdateState();

  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};
