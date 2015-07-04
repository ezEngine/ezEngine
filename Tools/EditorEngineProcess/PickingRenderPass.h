#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class ezPickingRenderPass : public ezRenderPipelinePass
{
public:
  ezPickingRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup);
  ~ezPickingRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

  struct Event
  {
    enum class Type
    {
      AfterOpaque,
      EndOfFrame,
    };

    Type m_Type;
  };

  ezEvent<const Event&> m_Events;

private:
  ezGALRenderTagetSetup m_RenderTargetSetup;
};
