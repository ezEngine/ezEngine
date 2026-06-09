#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezTerrainBrush3DVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezTerrainBrush3DVisualizerAdapter();
  ~ezTerrainBrush3DVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezEngineGizmoHandle m_hLinesInner;
  ezEngineGizmoHandle m_hLinesOuter;
};
