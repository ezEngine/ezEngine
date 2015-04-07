#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class MainRenderPass : public ezRenderPipelinePass
{
public:
  MainRenderPass();
  ~MainRenderPass();

  virtual void Execute(const ezRenderViewContext& renderContext) override;

private:
  ezRenderer* m_pMeshRenderer;
};