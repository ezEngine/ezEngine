#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

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

  ezVec3 GetTranslation();
  ezQuat GetRotation();
  ezVec3 GetScale();

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezManipulatorScaleGizmo m_ScaleGizmo;
  ezVec3 m_vOldScale;
};
