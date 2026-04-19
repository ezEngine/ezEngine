#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

/// Visualizer adapter for ezPointLightComponent.
///
/// Always shows a sphere gizmo at the light's range. If either Length or Radius is non-zero, also
/// shows a wireframe capsule so that tube (capsule) area lights are distinguishable from plain point lights.
class ezPointLightVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezPointLightVisualizerAdapter();
  ~ezPointLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fEffectiveRange = 1.0f;
  float m_fLength = 0.0f;
  float m_fRadius = 0.0f;
  bool m_bIsTube = false;

  ezEngineGizmoHandle m_hRangeGizmo;
  ezEngineGizmoHandle m_hCapsuleL;
  ezEngineGizmoHandle m_hCapsuleM;
  ezEngineGizmoHandle m_hCapsuleR;
};
