#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class ezTerrainBrush2DVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezTerrainBrush2DVisualizerAdapter();
  ~ezTerrainBrush2DVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezEngineGizmoHandle m_hLinesInner;
  ezEngineGizmoHandle m_hLinesOuter;
};
