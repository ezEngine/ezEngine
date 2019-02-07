#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>

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
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ezTranslateGizmo m_TranslateGizmo;
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

private:
  ezDragToPositionGizmo m_DragToPosGizmo;
};

