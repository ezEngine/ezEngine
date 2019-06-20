#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezPreferences;
struct ezGameObjectEvent;
class ezGameObjectDocument;
///
class EZ_EDITORFRAMEWORK_DLL ezGameObjectDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping, const char* szPath);
  static void MapMenuSimulationSpeed(const char* szMapping, const char* szPath);

  static void MapToolbarActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hGameObjectCategory;
  static ezActionDescriptorHandle s_hRenderSelectionOverlay;
  static ezActionDescriptorHandle s_hRenderVisualizers;
  static ezActionDescriptorHandle s_hRenderShapeIcons;
  static ezActionDescriptorHandle s_hRenderGrid;
  static ezActionDescriptorHandle s_hAddAmbientLight;
  static ezActionDescriptorHandle s_hSimulationSpeedMenu;
  static ezActionDescriptorHandle s_hSimulationSpeed[10];
  static ezActionDescriptorHandle s_hCameraSpeed;
};

///
class EZ_EDITORFRAMEWORK_DLL ezGameObjectDocumentAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectDocumentAction, ezButtonAction);

public:

  enum class ActionType
  {
    RenderSelectionOverlay,
    RenderVisualizers,
    RenderShapeIcons,
    RenderGrid,
    AddAmbientLight,
    SimulationSpeed,
  };

  ezGameObjectDocumentAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~ezGameObjectDocumentAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SceneEventHandler(const ezGameObjectEvent& e);
  void OnPreferenceChange(ezPreferences* pref);

  float m_fSimSpeed;
  ezGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};


class EZ_EDITORFRAMEWORK_DLL ezCameraSpeedSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCameraSpeedSliderAction, ezSliderAction);

public:

  enum class ActionType
  {
    CameraSpeed,
  };

  ezCameraSpeedSliderAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezCameraSpeedSliderAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);
  void UpdateState();

  ezGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};






