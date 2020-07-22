#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezGizmoEvent;

class ezSphereVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezSphereVisualizerAdapter();
  ~ezSphereVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_Scale;
  ezVec3 m_vPositionOffset;
  ezEngineGizmoHandle m_Gizmo;
};
