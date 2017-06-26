#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class ezSceneContext;

class ezPickingRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPickingRenderPass, ezRenderPipelinePass);

public:
  ezPickingRenderPass();
  ~ezPickingRenderPass();

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

  ezGALTextureHandle GetPickingIdRT() const;
  ezGALTextureHandle GetPickingDepthRT() const;

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual void ReadBackProperties(ezView* pView) override;

  ezViewRenderMode::Enum m_ViewRenderMode;
  bool m_bPickSelected;

  ezVec2 m_PickingPosition;
  ezUInt32 m_PickingIdOut;
  float m_PickingDepthOut;
  ezUInt32 m_uiWindowWidth;
  ezUInt32 m_uiWindowHeight;

private:
  void CreateTarget();
  void DestroyTarget();

private:
  ezRectFloat m_TargetRect;

  ezSceneContext* m_pSceneContext;
  ezGALTextureHandle m_hPickingIdRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTagetSetup m_RenderTargetSetup;


  /// we need this matrix to compute the world space position of picked pixels
  ezMat4 m_PickingInverseViewProjectionMatrix;

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  ezDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the ezComponent, etc. that rendered to that pixel
  ezDynamicArray<ezUInt32> m_PickingResultsID;
};
