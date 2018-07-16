#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezGizmoEvent;

class ezCylinderVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezCylinderVisualizerAdapter();
  ~ezCylinderVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius;
  float m_fHeight;
  ezVec3 m_vPositionOffset;

  ezEngineGizmoHandle m_Cylinder;
};
