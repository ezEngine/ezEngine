#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezBoxReflectionProbeVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezBoxReflectionProbeVisualizerAdapter();
  ~ezBoxReflectionProbeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezVec3 m_Scale;
  ezVec3 m_vPositionOffset;
  ezQuat m_Rotation;

  ezEngineGizmoHandle m_Gizmo;
};
