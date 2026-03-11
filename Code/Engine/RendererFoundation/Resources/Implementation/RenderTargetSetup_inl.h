
#pragma once

ezUInt8 ezGALRenderingSetup::GetColorTargetCount() const
{
  return m_RenderPass.m_uiRTCount;
}

const ezColor& ezGALRenderingSetup::GetClearColor(ezUInt8 uiIndex) const
{
  EZ_ASSERT_DEBUG(uiIndex < m_RenderPass.m_uiRTCount, "Render target at index {} does no exist, there are only {} render targets. Call GetRenderTargetCount first to determine max render targets.", uiIndex, m_RenderPass.m_uiRTCount);
  return m_ClearColor[uiIndex];
}

bool ezGALRenderingSetup::HasDepthStencilTarget() const
{
  return !m_FrameBuffer.m_hDepthTarget.IsInvalidated();
}

float ezGALRenderingSetup::GetClearDepth() const
{
  EZ_ASSERT_DEBUG(HasDepthStencilTarget(), "No depth target exists, check HasDepthStencilTarget() first.");
  return m_fClearDepth;
}

ezUInt8 ezGALRenderingSetup::GetClearStencil() const
{
  EZ_ASSERT_DEBUG(HasDepthStencilTarget(), "No depth target exists, check HasDepthStencilTarget() first.");
  return m_uiClearStencil;
}

const ezGALRenderPassDescriptor& ezGALRenderingSetup::GetRenderPass() const
{
  return m_RenderPass;
}
const ezGALFrameBufferDescriptor& ezGALRenderingSetup::GetFrameBuffer() const
{
  return m_FrameBuffer;
}
