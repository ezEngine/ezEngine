#pragma once

#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class ezQtGameObjectDocumentWindow;
class ezPreferences;

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezTranslateGizmoEditTool();
  ~ezTranslateGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ezTranslateGizmo m_TranslateGizmo;
  enum GridPlane
  {
    X,
    Y,
    Z
  };

  GridPlane m_GridPlane = GridPlane::Z;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezRotateGizmoEditTool();
  ~ezRotateGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  ezRotateGizmo m_RotateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezScaleGizmoEditTool();
  ~ezScaleGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalSpaceOnly; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  ezScaleGizmo m_ScaleGizmo;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezDragToPositionGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragToPositionGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezDragToPositionGizmoEditTool();
  ~ezDragToPositionGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  ezDragToPositionGizmo m_DragToPosGizmo;
};
