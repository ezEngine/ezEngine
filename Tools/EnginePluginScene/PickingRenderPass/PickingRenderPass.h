#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezSceneContext;

class ezPickingRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPickingRenderPass, ezRenderPipelinePass);

public:
  ezPickingRenderPass(ezSceneContext* pSceneContext, const ezGALRenderTagetSetup& RenderTargetSetup);
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
  ezSceneContext* m_pSceneContext;
  bool m_bEnable;
  ezGALRenderTagetSetup m_RenderTargetSetup;
};
