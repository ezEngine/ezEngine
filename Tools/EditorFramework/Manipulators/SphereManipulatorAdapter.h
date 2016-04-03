#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

struct ezGizmoEvent;

class ezSphereManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezSphereManipulatorAdapter();
  ~ezSphereManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  ezSphereGizmo m_Gizmo;
};