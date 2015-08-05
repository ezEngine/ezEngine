#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

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
  static ezActionDescriptorHandle s_hDragToPositionGizmo;
  static ezActionDescriptorHandle s_hWorldSpace;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoAction);
public:

  enum class ActionType
  {
    GizmoNone,
    GizmoTranslate,
    GizmoRotate,
    GizmoScale,
    GizmoDragToPosition,
    GizmoToggleWorldSpace,
  };

  ezGizmoAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezGizmoAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void UpdateState();
  void SceneEventHandler(const ezSceneDocument::SceneEvent& e);

  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};



class EZ_EDITORPLUGINSCENE_DLL ezRotateGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmoAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static void SetCurrentSnappingValue(float f);
  static float GetCurrentSnappingValue() { return s_fCurrentSnappingValue; }

private:
  static ezActionDescriptorHandle s_hSnappingValueMenu;
  static ezActionDescriptorHandle s_hSnappingValues[11];

public:
  enum class ActionType
  {
    SetSnappingAngle,
  };

  ezRotateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue);
  ~ezRotateGizmoAction();

  virtual void Execute(const ezVariant& value) override;

  struct Event
  {
    enum class Type
    {
      SnapppingAngleChanged,
    };

    Type m_Type;
  };

  static ezEvent<const Event&> s_Events;

private:
  void EventHandler(const Event& e);

  float m_fSnappingValue;
  ActionType m_Type;

  static float s_fCurrentSnappingValue;
};


class EZ_EDITORPLUGINSCENE_DLL ezScaleGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmoAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static void SetCurrentSnappingValue(float f);
  static float GetCurrentSnappingValue() { return s_fCurrentSnappingValue; }

private:
  static ezActionDescriptorHandle s_hSnappingValueMenu;
  static ezActionDescriptorHandle s_hSnappingValues[8];

public:
  enum class ActionType
  {
    SetSnappingValue,
  };

  ezScaleGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue);
  ~ezScaleGizmoAction();

  virtual void Execute(const ezVariant& value) override;

  struct Event
  {
    enum class Type
    {
      SnapppingValueChanged,
    };

    Type m_Type;
  };

  static ezEvent<const Event&> s_Events;

private:
  void EventHandler(const Event& e);

  float m_fSnappingValue;
  ActionType m_Type;

  static float s_fCurrentSnappingValue;
};


class EZ_EDITORPLUGINSCENE_DLL ezTranslateGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmoAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static void SetCurrentSnappingValue(float f);
  static float GetCurrentSnappingValue() { return s_fCurrentSnappingValue; }

private:
  static ezActionDescriptorHandle s_hSnappingValueMenu;
  static ezActionDescriptorHandle s_hSnappingValues[9];
  static ezActionDescriptorHandle s_hSnapPivotToGrid;
  static ezActionDescriptorHandle s_hSnapObjectsToGrid;

public:
  enum class ActionType
  {
    SetSnappingValue,
    SnapSelectionPivotToGrid,
    SnapEachSelectedObjectToGrid,
  };

  ezTranslateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue);
  ~ezTranslateGizmoAction();

  virtual void Execute(const ezVariant& value) override;

  struct Event
  {
    enum class Type
    {
      SnapppingValueChanged,
    };

    Type m_Type;
  };

  static ezEvent<const Event&> s_Events;

private:
  void EventHandler(const Event& e);

  ezSceneDocument* m_pSceneDocument;
  float m_fSnappingValue;
  ActionType m_Type;

  static float s_fCurrentSnappingValue;
};




