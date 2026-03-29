#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezTubeLightVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezTubeLightVisualizerAdapter();
  ~ezTubeLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale = 1.0f;
  float m_fLength = 1.0f;
  float m_fRadius = 0.05f;
  ezEngineGizmoHandle m_hCapsuleL;
  ezEngineGizmoHandle m_hCapsuleM;
  ezEngineGizmoHandle m_hCapsuleR;
  ezEngineGizmoHandle m_hRangeGizmo;
};
