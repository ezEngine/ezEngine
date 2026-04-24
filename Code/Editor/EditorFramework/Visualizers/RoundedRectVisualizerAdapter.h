#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct ezGizmoEvent;

class ezRoundedRectVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezRoundedRectVisualizerAdapter();
  ~ezRoundedRectVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezEngineGizmoHandle m_hLinesInner;
  ezEngineGizmoHandle m_hLinesOuter;
};
