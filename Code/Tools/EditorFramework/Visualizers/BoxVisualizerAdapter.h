#pragma once 

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>

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

  ezVec3 m_Scale;
  ezVec3 m_vPositionOffset;
  ezEngineGizmoHandle m_Gizmo;
};
