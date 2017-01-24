#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Basics.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class EZ_RENDERERCORE_DLL ezScreenSpaceAmbientOcclusionPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScreenSpaceAmbientOcclusionPass, ezRenderPipelinePass);

public:
  ezScreenSpaceAmbientOcclusionPass();
  ~ezScreenSpaceAmbientOcclusionPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs);

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  ezUInt32 GetLineToLinePixelOffset() const { return m_uiLineToLinePixelOffset; }
  void SetLineToLinePixelOffset(ezUInt32 uiPixelOffset);
  ezUInt32 GetLineSamplePixelOffset() const { return m_uiLineSamplePixelOffsetFactor; }
  void SetLineSamplePixelOffset(ezUInt32 uiPixelOffset);

  // Factor used for depth cutoffs (determines when a depth difference is too large to be considered)
  float GetDepthCutoffDistance() const;
  void SetDepthCutoffDistance(float fDepthCutoffDistance);

  // Determines how quickly the occlusion falls of.
  float GetOcclusionFalloff() const;
  void SetOcclusionFalloff(float fFalloff);


protected:

  /// Destroys all GPU data that might have been created in in SetupLineSweepData
  void DestroyLineSweepData();
  void SetupLineSweepData(const ezVec2I32& imageResolution);


  void AddLinesForDirection(const ezVec2I32& imageResolution, const ezVec2I32& sampleDir, ezUInt32 lineIndex, ezDynamicArray<LineInstruction>& outinLineInstructions, ezUInt32& outinTotalNumberOfSamples);

  ezInputNodePin m_PinDepthInput;
  ezOutputNodePin m_PinOutput;

  ezConstantBufferStorageHandle m_hLineSweepCB;

  bool m_bSweepDataDirty;

  /// Output of the line sweep pass.
  ezGALBufferHandle m_hLineSweepOutputBuffer;
  ezGALUnorderedAccessViewHandle m_hLineSweepOutputUAV;
  ezGALResourceViewHandle m_hLineSweepOutputSRV;

  /// Structured buffer containing instructions for every single line to trace.
  ezGALBufferHandle m_hLineInfoBuffer;
  ezGALResourceViewHandle m_hLineSweepInfoSRV;

  /// Total number of lines to be traced.
  ezUInt32 m_numSweepLines;

  ezInt32 m_uiLineToLinePixelOffset;
  ezInt32 m_uiLineSamplePixelOffsetFactor;
  bool m_bDistributedGathering;

  ezShaderResourceHandle m_hShaderLineSweep;
  ezShaderResourceHandle m_hShaderGather;
  ezShaderResourceHandle m_hShaderAverage;
};
