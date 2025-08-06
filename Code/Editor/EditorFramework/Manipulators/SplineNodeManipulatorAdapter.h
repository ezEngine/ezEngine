#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
#include <EditorFramework/Gizmos/SplineTangentGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

/// \brief Makes tangents of spline nodes editable in the editor.
///
/// Enabled by attaching the ezSplineNodeManipulatorAttribute.
class ezSplineNodeManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezSplineNodeManipulatorAdapter();
  ~ezSplineNodeManipulatorAdapter();

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void TangentGizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();

  ezSpline m_Spline;
  ezUInt32 m_uiNodeIndex = ezInvalidIndex;

  ezVec3 m_vLastTangentIn;
  ezVec3 m_vLastTangentOut;

  ezSplineTangentGizmo m_TangentInGizmo;
  ezSplineTangentGizmo m_TangentOutGizmo;
};
