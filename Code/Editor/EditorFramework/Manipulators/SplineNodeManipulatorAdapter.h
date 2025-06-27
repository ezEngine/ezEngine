#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Core/Graphics/Spline.h>
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

  virtual void UpdateGizmoTransform() override;

  void BuildSpline();
  void ConfigureGizmos();

  ezSpline m_Spline;
};
