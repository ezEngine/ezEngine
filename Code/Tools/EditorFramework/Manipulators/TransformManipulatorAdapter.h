#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>

struct ezGizmoEvent;

class ezTransformManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezTransformManipulatorAdapter();
  ~ezTransformManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezManipulatorScaleGizmo m_ScaleGizmo;
  ezVec3 m_vOldScale;
};
