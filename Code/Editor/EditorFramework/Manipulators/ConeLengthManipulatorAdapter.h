#pragma once 

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>

struct ezGizmoEvent;

class ezConeLengthManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezConeLengthManipulatorAdapter();
  ~ezConeLengthManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezConeLengthGizmo m_Gizmo;
};
