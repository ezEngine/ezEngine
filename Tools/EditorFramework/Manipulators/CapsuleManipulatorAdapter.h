#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>

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