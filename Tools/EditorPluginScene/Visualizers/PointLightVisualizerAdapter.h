#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

class ezPointLightVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezPointLightVisualizerAdapter();
  ~ezPointLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale;
  ezEngineGizmoHandle m_Gizmo;
};
