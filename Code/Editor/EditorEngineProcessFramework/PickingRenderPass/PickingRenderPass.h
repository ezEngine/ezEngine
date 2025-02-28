#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezPickingRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPickingRenderPass, ezRenderPipelinePass);

public:
  ezPickingRenderPass();
  ~ezPickingRenderPass();

  ezGALTextureHandle GetPickingIdRT() const;
  ezGALTextureHandle GetPickingDepthRT() const;

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(
    const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual void ReadBackProperties(ezView* pView) override;

  bool m_bPickSelected = true;
  bool m_bPickTransparent = true;

  ezVec2 m_PickingPosition = ezVec2(-1);
  ezUInt32 m_PickingIdOut = 0;
  float m_PickingDepthOut = 0.0f;
  ezVec2 m_MarqueePickPosition0 = ezVec2(-1);
  ezVec2 m_MarqueePickPosition1 = ezVec2(-1);
  ezUInt32 m_uiMarqueeActionID = 0xFFFFFFFF; // used to prevent reusing an old result for a new marquee action
  ezUInt32 m_uiWindowWidth = 0;
  ezUInt32 m_uiWindowHeight = 0;

private:
  void CreateTarget();
  void DestroyTarget();

  void ReadBackPropertiesSinglePick(ezView* pView);
  void ReadBackPropertiesMarqueePick(ezView* pView);

  void ProcessPickingRenderData(ezExtractedRenderData& extractedRenderData);

private:
  ezRectFloat m_TargetRect;
  const ezRTTI* m_pGridRenderDataType = nullptr;

  ezGALTextureHandle m_hPickingIdRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTargetSetup m_RenderTargetSetup;

  ezHashSet<ezGameObjectHandle> m_SelectionSet;

  // Readback
  struct PickingReadback
  {
    ezGALReadbackTextureHelper m_PickingReadback;
    ezGALReadbackTextureHelper m_PickingDepthReadback;

    bool m_bReadbackInProgress = false;
    ezUInt32 m_uiWindowWidth = 0;
    ezUInt32 m_uiWindowHeight = 0;
    /// we need this matrix to compute the world space position of picked pixels
    ezMat4 m_mPickingInverseViewProjectionMatrix = ezMat4::MakeZero();
  };

  PickingReadback m_PendingReadback;

  // Picking Results
  ezMat4 m_mPickingInverseViewProjectionMatrix = ezMat4::MakeZero();
  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  ezDynamicArray<float> m_PickingResultsDepth;
  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the ezComponent, etc. that rendered to that pixel
  ezDynamicArray<ezUInt32> m_PickingResultsID;

  ezUInt32 m_uiProcessorId = ezInvalidIndex;
};
