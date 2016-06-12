#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezMaterialContext;

class ezMaterialViewContext : public ezEngineProcessViewContext
{
public:
  ezMaterialViewContext(ezMaterialContext* pMaterialContext);
  ~ezMaterialViewContext();

protected:
  virtual bool IsDefaultRenderPipeline(ezRenderPipelineResourceHandle hPipeline) override;
  virtual ezRenderPipelineResourceHandle CreateDefaultRenderPipeline() override;
  virtual ezView* CreateView() override;

  ezMaterialContext* m_pMaterialContext;
};

