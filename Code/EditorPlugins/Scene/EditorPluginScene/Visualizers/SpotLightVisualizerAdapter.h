#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezSpotLightVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezSpotLightVisualizerAdapter();
  ~ezSpotLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale = 1.0f;
  float m_fAngleScale = 1.0f;
  float m_fRadius = 0.0f;
  ezEngineGizmoHandle m_hGizmo;
  ezEngineGizmoHandle m_hRadiusGizmo;
};
