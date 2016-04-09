#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

struct ezGizmoEvent;

class ezCapsuleVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezCapsuleVisualizerAdapter();
  ~ezCapsuleVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezMat4 m_ScaleCylinder;
  ezMat4 m_ScaleSphereTop;
  ezMat4 m_ScaleSphereBottom;

  ezEngineGizmoHandle m_SphereTop;
  ezEngineGizmoHandle m_SphereBottom;
  ezEngineGizmoHandle m_Cylinder;
};