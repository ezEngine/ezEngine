
#pragma once

#include <Foundation/Math/Rect.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class EZ_RENDERERFOUNDATION_DLL ezGALRenderCommandEncoder : public ezGALCommandEncoder
{
public:
  ezGALRenderCommandEncoder(ezGALDevice& device, ezGALCommandEncoderRenderState& renderState, ezGALCommandEncoderCommonPlatformInterface& commonImpl, ezGALCommandEncoderRenderPlatformInterface& renderImpl);
  virtual ~ezGALRenderCommandEncoder();

  // Draw functions

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, ezUInt8 uiStencilClear = 0x0u);

  void Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex);
  void DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex);
  void DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex);
  void DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);
  void DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex);
  void DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);
  void DrawAuto();

  void BeginStreamOut();
  void EndStreamOut();

  // State functions

  void SetIndexBuffer(ezGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer);
  void SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  ezGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_RenderState.m_Topology; }
  void SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology);

  void SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor = ezColor::White, ezUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const ezRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const ezRectU32& rect);

  void SetStreamOutBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer, ezUInt32 uiOffset);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  ezUInt32 m_uiDrawCalls = 0;

  ezGALCommandEncoderRenderState& m_RenderState;

  ezGALCommandEncoderRenderPlatformInterface& m_RenderImpl;
};
