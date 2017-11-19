#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

enum class ActiveGizmo;
class ezGameObjectDocument;
struct ezSnapProviderEvent;
struct ezGameObjectEvent;

///
class EZ_EDITORFRAMEWORK_DLL ezTransformGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping, const char* szPath);
  static void MapToolbarActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hGizmoCategory;
  static ezActionDescriptorHandle s_hGizmoMenu;
  static ezActionDescriptorHandle s_hNoGizmo;
  static ezActionDescriptorHandle s_hTranslateGizmo;
  static ezActionDescriptorHandle s_hRotateGizmo;
  static ezActionDescriptorHandle s_hScaleGizmo;
  static ezActionDescriptorHandle s_hDragToPositionGizmo;
  static ezActionDescriptorHandle s_hWorldSpace;
  static ezActionDescriptorHandle s_hMoveParentOnly;
};

///
class EZ_EDITORFRAMEWORK_DLL ezGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoAction, ezButtonAction);
public:
  ezGizmoAction(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType);
  ~ezGizmoAction();

  virtual void Execute(const ezVariant& value) override;

protected:
  void UpdateState();
  void GameObjectEventHandler(const ezGameObjectEvent& e);

  ezGameObjectDocument* m_pGameObjectDocument = nullptr;
  const ezRTTI* m_pGizmoType = nullptr;
};

///
class EZ_EDITORFRAMEWORK_DLL ezToggleWorldSpaceGizmo : public ezGizmoAction
{
public:
  ezToggleWorldSpaceGizmo(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType);
  virtual void Execute(const ezVariant& value) override;
};

///
class EZ_EDITORFRAMEWORK_DLL ezTransformGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransformGizmoAction, ezButtonAction);
public:

  enum class ActionType
  {
    GizmoToggleWorldSpace,
    GizmoToggleMoveParentOnly,
  };

  ezTransformGizmoAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezTransformGizmoAction();

  virtual void Execute(const ezVariant& value) override;
  void GameObjectEventHandler(const ezGameObjectEvent& e);

private:
  void UpdateState();

  ezGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};

///
class EZ_EDITORFRAMEWORK_DLL ezRotateGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmoAction, ezButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

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

private:
  void EventHandler(const ezSnapProviderEvent& e);

  float m_fSnappingValue;
  ActionType m_Type;
};

///
class EZ_EDITORFRAMEWORK_DLL ezScaleGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmoAction, ezButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

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

private:
  void EventHandler(const ezSnapProviderEvent& e);

  float m_fSnappingValue;
  ActionType m_Type;
};

///
class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmoAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmoAction, ezButtonAction);

public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

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

private:
  void EventHandler(const ezSnapProviderEvent& e);

  const ezGameObjectDocument* m_pSceneDocument;
  float m_fSnappingValue;
  ActionType m_Type;

};
