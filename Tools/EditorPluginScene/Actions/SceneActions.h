#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class ezPreferences;

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();
  static void MapToolbarActions();

  static ezActionDescriptorHandle s_hSceneCategory;
  static ezActionDescriptorHandle s_hExportScene;
  static ezActionDescriptorHandle s_hRunScene;
  static ezActionDescriptorHandle s_hGameModeSimulate;
  static ezActionDescriptorHandle s_hRenderSelectionOverlay;
  static ezActionDescriptorHandle s_hRenderVisualizers;
  static ezActionDescriptorHandle s_hRenderShapeIcons;
  static ezActionDescriptorHandle s_hRenderGrid;
  static ezActionDescriptorHandle s_hAddAmbientLight;
  static ezActionDescriptorHandle s_hSimulationSpeedMenu;
  static ezActionDescriptorHandle s_hSimulationSpeed[10];
  static ezActionDescriptorHandle s_hGameModePlay;
  static ezActionDescriptorHandle s_hGameModeStop;

  static ezActionDescriptorHandle s_hCameraSpeed;
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
    RenderSelectionOverlay,
    RenderVisualizers,
    RenderShapeIcons,
    RenderGrid,
    AddAmbientLight,
    SimulationSpeed,
    StartGameModePlay,
    StopGameMode,
  };

  ezSceneAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~ezSceneAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SceneEventHandler(const ezSceneDocumentEvent& e);
  void UpdateState();
  void OnPreferenceChange(ezPreferences* pref);

  float m_fSimSpeed;
  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};


class EZ_EDITORPLUGINSCENE_DLL ezSceneSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneSliderAction, ezSliderAction);

public:

  enum class ActionType
  {
    CameraSpeed,
  };

  ezSceneSliderAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezSceneSliderAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);
  void UpdateState();

  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};






