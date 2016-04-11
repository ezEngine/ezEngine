#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/ConeGizmo.h>

struct ezGizmoEvent;

class ezConeManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezConeManipulatorAdapter();
  ~ezConeManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezConeGizmo m_Gizmo;
};