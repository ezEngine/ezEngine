#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct ezGizmoEvent;

class ezNonUniformBoxManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezNonUniformBoxManipulatorAdapter();
  ~ezNonUniformBoxManipulatorAdapter();

  virtual void QueryGridSettings(ezGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezNonUniformBoxGizmo m_Gizmo;
};
