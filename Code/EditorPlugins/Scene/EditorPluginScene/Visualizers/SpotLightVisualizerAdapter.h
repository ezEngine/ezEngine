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

  float m_fScale;
  float m_fAngleScale;
  ezEngineGizmoHandle m_hGizmo;
};
