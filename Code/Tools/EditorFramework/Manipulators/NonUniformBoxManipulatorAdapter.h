#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>

struct ezGizmoEvent;

class ezNonUniformBoxManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezNonUniformBoxManipulatorAdapter();
  ~ezNonUniformBoxManipulatorAdapter();

  virtual void QueryGridSettings(ezGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const ezGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  ezNonUniformBoxGizmo m_Gizmo;
};
