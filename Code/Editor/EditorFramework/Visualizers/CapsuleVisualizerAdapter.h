#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezGizmoEvent;

class ezCapsuleVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezCapsuleVisualizerAdapter();
  ~ezCapsuleVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius = 0.0f;
  float m_fHeight = 0.0f;
  ezBitflags<ezVisualizerAnchor> m_Anchor;

  ezEngineGizmoHandle m_hSphereTop;
  ezEngineGizmoHandle m_hSphereBottom;
  ezEngineGizmoHandle m_hCylinder;
};
