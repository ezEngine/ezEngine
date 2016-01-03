#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezSceneContext;

class ezPickingRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPickingRenderPass, ezRenderPipelinePass);

public:
  ezPickingRenderPass();
  ~ezPickingRenderPass();

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  void SetEnabled(bool b) { m_bEnable = b; }
  ezGALTextureHandle GetPickingIdRT() const;
  ezGALTextureHandle GetPickingDepthRT() const;

  virtual bool GetRenderTargetDescriptions(const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void SetRenderTargets(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
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
  void CreateTarget();
  void DestroyTarget();

private:
  bool m_bEnable;
  ezSceneContext* m_pSceneContext;
  ezGALTextureHandle m_hPickingIdRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTargetViewHandle m_hPickingIdRTV;
  ezGALRenderTargetViewHandle m_hPickingDepthDSV;
  ezGALRenderTagetSetup m_RenderTargetSetup;
};
