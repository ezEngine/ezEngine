#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezGizmoEvent;

class ezBoxVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezBoxVisualizerAdapter();
  ~ezBoxVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezVec3 m_vScale;
  ezVec3 m_vPositionOffset;
  ezQuat m_qRotation;
  ezBitflags<ezVisualizerAnchor> m_Anchor;
  ezEngineGizmoHandle m_hGizmo;
};
