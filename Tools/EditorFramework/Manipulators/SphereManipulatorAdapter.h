#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class ezSphereManipulatorAdapter : public ezManipulatorAdapter
{
public:
  ezSphereManipulatorAdapter();
  ~ezSphereManipulatorAdapter();

protected:
  virtual void Finalize() override;

  virtual void Update() override;

  ezSphereGizmo m_Gizmo;
};