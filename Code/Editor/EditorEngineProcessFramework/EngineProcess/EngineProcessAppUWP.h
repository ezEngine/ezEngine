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
};
