#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

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
