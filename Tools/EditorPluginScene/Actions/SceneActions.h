#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();
  static void MapToolbarActions();

  static ezActionDescriptorHandle s_hSceneCategory;
  static ezActionDescriptorHandle s_hUpdatePrefabs;
  static ezActionDescriptorHandle s_hExportScene;
  static ezActionDescriptorHandle s_hRunScene;
  static ezActionDescriptorHandle s_hGameModeSimulate;
  static ezActionDescriptorHandle s_hRenderSelectionOverlay;
  static ezActionDescriptorHandle s_hRenderShapeIcons;
  static ezActionDescriptorHandle s_hSimulationSpeedMenu;
  static ezActionDescriptorHandle s_hSimulationSpeed[10];
  static ezActionDescriptorHandle s_hGameModePlay;
  static ezActionDescriptorHandle s_hGameModeStop;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneAction, ezButtonAction);

public:

  enum class ActionType
  {
    UpdatePrefabs,
    ExportScene,
    RunScene,
    StartGameModeSimulate,
    RenderSelectionOverlay,
    RenderShapeIcons,
    SimulationSpeed,
    StartGameModePlay,
    StopGameMode,
  };

  ezSceneAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~ezSceneAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SceneEventHandler(const ezSceneDocument::SceneEvent& e);
  void UpdateState();

  float m_fSimSpeed;
  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};




