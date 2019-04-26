#pragma once 

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/BoxGizmo.h>

struct ezGizmoEvent;

class ezBoxManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezBoxManipulatorAdapter();
  ~ezBoxManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezVec3 m_vPositionOffset;
  ezQuat m_Rotation;
  ezBoxGizmo m_Gizmo;
};
