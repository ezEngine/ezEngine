#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

/// \brief Makes a spline tangent editable in the editor.
///
/// Enabled by attaching the ezSplineTangentManipulatorAttribute.
class ezSplineTangentManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezSplineTangentManipulatorAdapter();
  ~ezSplineTangentManipulatorAdapter();

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void TangentGizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();

  bool CustomTangentsLinked() const;

  ezSpline m_Spline;
  ezUInt32 m_uiNodeIndex = ezInvalidIndex;
  bool m_bIsTangentIn = false;

  ezVec3 m_vLastTangent;

  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;
};
