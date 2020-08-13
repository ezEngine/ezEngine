#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

class ezCapsuleManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezCapsuleManipulatorAdapter();
  ~ezCapsuleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezCapsuleGizmo m_Gizmo;
};
