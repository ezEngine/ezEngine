#pragma once

#include <RendererFoundation/Resources/RenderTargetConfig.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class ezPickingRenderPass : public ezRenderPipelinePass
{
public:
  ezPickingRenderPass(ezGALRenderTargetConfigHandle hRTConfig);
  ~ezPickingRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

  struct Event
  {

  };

  ezEvent<const Event&> m_Events;

private:
  ezGALRenderTargetConfigHandle m_hRTConfig;
};
