#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

struct ezGizmoEvent;

class ezBoxVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezBoxVisualizerAdapter();
  ~ezBoxVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezVec3 m_Scale;
  ezEngineGizmoHandle m_Gizmo;
};