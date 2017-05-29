#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

struct ezGizmoEvent;

class ezSphereVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezSphereVisualizerAdapter();
  ~ezSphereVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_Scale;
  ezEngineGizmoHandle m_Gizmo;
};