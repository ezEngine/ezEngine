#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraphContext.h>

ezGALTextureHandle ezRenderGraphContext::ResolveTexture(ezRenderGraphTextureHandle hTexture) const
{
  ezUInt32 uiInstanceId = hTexture.GetInternalID().m_InstanceIndex;
  EZ_ASSERT_DEBUG(uiInstanceId < m_pTextureToResolvedTexture->GetCount(), "Invalid texture ID");
  const ezUInt16 uiResolvedTextureIndex = (*m_pTextureToResolvedTexture)[uiInstanceId];
  return (*m_pResolvedTextures)[uiResolvedTextureIndex];
}

ezGALBufferHandle ezRenderGraphContext::ResolveBuffer(ezRenderGraphBufferHandle hBuffer) const
{
  ezUInt32 uiInstanceId = hBuffer.GetInternalID().m_InstanceIndex;
  EZ_ASSERT_DEBUG(uiInstanceId < m_pBufferToResolvedBuffer->GetCount(), "Invalid Buffer ID");
  const ezUInt16 uiResolvedBufferIndex = (*m_pBufferToResolvedBuffer)[uiInstanceId];
  return (*m_pResolvedBuffers)[uiResolvedBufferIndex];
}

ezGALCommandEncoder* ezRenderGraphContext::GetCommandEncoder() const
{
  return m_pCommandEncoder;
}

ezGALDevice* ezRenderGraphContext::GetDevice() const
{
  return m_pDevice;
}

ezRenderContext* ezRenderGraphContext::GetRenderContext() const
{
  return m_pRenderContext;
}
