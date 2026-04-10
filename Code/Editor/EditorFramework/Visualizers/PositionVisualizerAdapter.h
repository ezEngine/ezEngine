#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class EZ_EDITORFRAMEWORK_DLL ezPositionVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezPositionVisualizerAdapter();
  ~ezPositionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  virtual void UpdateGizmoTransform() override;

  float m_fScale = 0.1f;
  ezVec3 m_vPosition;
  ezEngineGizmoHandle m_hGizmo;
};
