#pragma once 

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>

struct ezGizmoEvent;

class ezDirectionVisualizerAdapter : public ezVisualizerAdapter
{
public:
  ezDirectionVisualizerAdapter();
  ~ezDirectionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  ezEngineGizmoHandle m_Gizmo;
};
