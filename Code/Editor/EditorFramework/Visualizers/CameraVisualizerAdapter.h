#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezCameraVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezCameraVisualizerAdapter();
  ~ezCameraVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezTransform m_LocalTransformFrustum;
  ezTransform m_LocalTransformNearPlane;
  ezTransform m_LocalTransformFarPlane;
  ezEngineGizmoHandle m_hBoxGizmo;
  ezEngineGizmoHandle m_hFrustumGizmo;
  ezEngineGizmoHandle m_hNearPlaneGizmo;
  ezEngineGizmoHandle m_hFarPlaneGizmo;
};
