#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

/// Visualizer for directional lights.
///
/// Renders a cone along the light direction whose half-angle matches the authored source angle
/// (the "sun disc" size). The cone's length is purely symbolic since directional lights have no
/// position in the scene; only the opening angle carries meaning.
class ezDirectionalLightVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezDirectionalLightVisualizerAdapter();
  ~ezDirectionalLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fAngleScale = 0.0f;
  ezEngineGizmoHandle m_hGizmo;
};
