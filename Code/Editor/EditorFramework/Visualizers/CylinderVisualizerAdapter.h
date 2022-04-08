#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
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
  ezBitflags<ezVisualizerAnchor> m_Anchor;
  ezBasisAxis::Enum m_Axis;

  ezEngineGizmoHandle m_Cylinder;
};
