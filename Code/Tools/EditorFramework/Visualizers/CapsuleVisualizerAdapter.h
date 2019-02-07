#pragma once 

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>

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

  float m_fRadius;
  float m_fHeight;

  ezEngineGizmoHandle m_SphereTop;
  ezEngineGizmoHandle m_SphereBottom;
  ezEngineGizmoHandle m_Cylinder;
};
