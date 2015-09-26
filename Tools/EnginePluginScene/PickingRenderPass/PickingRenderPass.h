#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezPickingRenderPass : public ezRenderPipelinePass
{
public:
  ezPickingRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup);
  ~ezPickingRenderPass();

  void SetEnabled(bool b) { m_bEnable = b; }

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
  ezViewRenderMode::Enum m_ViewRenderMode;

private:
  bool m_bEnable;
  ezGALRenderTagetSetup m_RenderTargetSetup;
};
