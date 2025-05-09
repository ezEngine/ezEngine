#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Utilities/Stats.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void ezGALCommandEncoderRenderState::InvalidateState()
{
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
    m_hVertexBufferOffsets[i] = 0;
  }
  m_hIndexBuffer.Invalidate();

  m_hGraphicsPipeline.Invalidate();
  m_hComputePipeline.Invalidate();
  m_uiStencilRefValue = 0;

  m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = ezMath::MaxValue<float>();
  m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
}

void ezGALCommandEncoderStats::Reset()
{
  ezMemoryUtils::ZeroFill<ezGALCommandEncoderStats>(static_cast<ezGALCommandEncoderStats*>(this), 1);
}

void ezGALCommandEncoderStats::SetStatistics()
{
  ezStats::SetStat("GalCommandEncoder/Operations/InsertTimestamp", m_uiInsertTimestamp);
  ezStats::SetStat("GalCommandEncoder/Operations/BeginOcclusionQuery", m_uiBeginOcclusionQuery);
  ezStats::SetStat("GalCommandEncoder/Operations/InsertFence", m_uiInsertFence);
  ezStats::SetStat("GalCommandEncoder/Operations/CopyBuffer", m_uiCopyBuffer);
  ezStats::SetStat("GalCommandEncoder/Operations/UpdateBuffer", m_uiUpdateBuffer);
  ezStats::SetStat("GalCommandEncoder/Operations/CopyTexture", m_uiCopyTexture);
  ezStats::SetStat("GalCommandEncoder/Operations/UpdateTexture", m_uiUpdateTexture);
  ezStats::SetStat("GalCommandEncoder/Operations/ResolveTexture", m_uiResolveTexture);
  ezStats::SetStat("GalCommandEncoder/Operations/ReadbackTexture", m_uiReadbackTexture);
  ezStats::SetStat("GalCommandEncoder/Operations/ReadbackBuffer", m_uiReadbackBuffer);
  ezStats::SetStat("GalCommandEncoder/Operations/Flush", m_uiFlush);
  ezStats::SetStat("GalCommandEncoder/Compute/BeginCompute", m_uiBeginCompute);
  ezStats::SetStat("GalCommandEncoder/Compute/Dispatch", m_uiDispatch);
  ezStats::SetStat("GalCommandEncoder/Render/BeginRendering", m_uiBeginRendering);
  ezStats::SetStat("GalCommandEncoder/Render/Clear", m_uiClear);
  ezStats::SetStat("GalCommandEncoder/Render/Draw", m_uiDraw);
  ezStats::SetStat("GalCommandEncoder/States/SetIndexBuffer", m_uiSetIndexBuffer);
  ezStats::SetStat("GalCommandEncoder/States/SetVertexBuffer", m_uiSetVertexBuffer);
  ezStats::SetStat("GalCommandEncoder/States/SetGraphicsPipeline", m_uiSetGraphicsPipeline);
  ezStats::SetStat("GalCommandEncoder/States/SetComputePipeline", m_uiSetComputePipeline);
  ezStats::SetStat("GalCommandEncoder/DynamicStates/SetViewport", m_uiSetViewport);
  ezStats::SetStat("GalCommandEncoder/DynamicStates/SetScissorRect", m_uiSetScissorRect);
  ezStats::SetStat("GalCommandEncoder/DynamicStates/SetStencilReference", m_uiSetStencilReference);
}

void ezGALCommandEncoderStats::operator+=(const ezGALCommandEncoderStats& rhs)
{
  const ezUInt32 uiCount = sizeof(ezGALCommandEncoderStats) / sizeof(ezUInt32);
  ezUInt32* pDest = reinterpret_cast<ezUInt32*>(this);
  const ezUInt32* pSource = reinterpret_cast<const ezUInt32*>(&rhs);
  for (int i = 0; i < uiCount; ++i)
  {
    pDest[i] += pSource[i];
  }
}
