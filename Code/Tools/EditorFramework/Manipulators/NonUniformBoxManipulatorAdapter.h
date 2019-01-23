#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>

struct ezGizmoEvent;

class ezNonUniformBoxManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezNonUniformBoxManipulatorAdapter();
  ~ezNonUniformBoxManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezNonUniformBoxGizmo m_Gizmo;
};
