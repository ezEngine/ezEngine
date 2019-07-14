#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezEditorEngineProcessAppUWP : public ezEditorEngineProcessApp
{
public:
  ezEditorEngineProcessAppUWP();
  ~ezEditorEngineProcessAppUWP();

  virtual ezViewHandle CreateRemoteWindowAndView(ezCamera* pCamera) override;

  virtual ezRenderPipelineResourceHandle CreateDefaultMainRenderPipeline() override;
  virtual ezRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline() override;

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  void SetAnchor(const ezVec3& position);
  void SetAnchor(const ezTransform& offset);
  void LoadAnchor();
  bool m_bAnchorLoaded = false;
#endif

};