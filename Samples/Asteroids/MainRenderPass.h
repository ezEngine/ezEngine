#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class MainRenderPass : public ezRenderPipelinePass
{
public:
  MainRenderPass();

  virtual void Execute() override;
};