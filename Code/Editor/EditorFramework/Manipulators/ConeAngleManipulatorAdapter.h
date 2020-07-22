#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

class ezConeAngleManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezConeAngleManipulatorAdapter();
  ~ezConeAngleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezConeAngleGizmo m_Gizmo;
};
