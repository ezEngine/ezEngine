
#include <RendererVulkanPCH.h>

#include <RendererVulkan/CommandEncoder/RenderCommandEncoderVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FenceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>

ezGALRenderCommandEncoderVulkan::ezGALRenderCommandEncoderVulkan(ezGALDevice& device)
  : SUPER(device)
{
}

ezGALRenderCommandEncoderVulkan::~ezGALRenderCommandEncoderVulkan() = default;

void ezGALRenderCommandEncoderVulkan::BeginEncode(vk::CommandBuffer& commandBuffer, const ezGALRenderingSetup& renderingSetup)
{
  m_pCommandBuffer = &commandBuffer;

  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = 0;
  // TODO: fill render pass create info
  // TODO: caching

  vk::RenderPass renderPass = m_vkDevice.createRenderPass(renderPassCreateInfo);

  vk::RenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo.renderPass = renderPass;

  m_pCommandBuffer->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

void ezGALRenderCommandEncoderVulkan::EndEncode()
{
  m_pCommandBuffer->endRenderPass();

  m_pCommandBuffer = nullptr;
}

void ezGALRenderCommandEncoderVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  // TODO:
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Draw functions

void ezGALRenderCommandEncoderVulkan::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCount, 1, uiStartVertex, 0);
}

void ezGALRenderCommandEncoderVulkan::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

#if 0 //EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    ezGALDeviceVulkan* pDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());
    if (pDevice->m_pDebug)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(pDevice->m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
#endif
}

void ezGALRenderCommandEncoderVulkan::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALRenderCommandEncoderVulkan::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALRenderCommandEncoderVulkan::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALRenderCommandEncoderVulkan::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALRenderCommandEncoderVulkan::DrawAutoPlatform()
{
  //FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALRenderCommandEncoderVulkan::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();
}

void ezGALRenderCommandEncoderVulkan::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALRenderCommandEncoderVulkan::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  // TODO index buffer needs to be in index buffer resource state

  if (pIndexBuffer != nullptr)
  {
    const ezGALBufferVulkan* pVulkanBuffer = static_cast<const ezGALBufferVulkan*>(pIndexBuffer);
    m_pCommandBuffer->bindIndexBuffer(pVulkanBuffer->GetVkBuffer(), 0, pVulkanBuffer->GetIndexType());
  }
  else
  {
    // TODO binding a null buffer is not allowed in vulkan
    EZ_ASSERT_NOT_IMPLEMENTED;
    //m_pCommandBuffer->bindIndexBuffer({}, 0, vk::IndexType::eUint16);
  }
}

void ezGALRenderCommandEncoderVulkan::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetVkBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALRenderCommandEncoderVulkan::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  if (pVertexDeclaration)
  {
    m_pCurrentVertexLayout = &static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration)->GetInputLayout();
    m_bPipelineStateDirty = true;
  }
  // TODO defer to pipeline update
  //m_pDXContext->IASetInputLayout(
  //  pVertexDeclaration != nullptr ? static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}


static const vk::PrimitiveTopology GALTopologyToVulkan[ezGALPrimitiveTopology::ENUM_COUNT] = {
  vk::PrimitiveTopology::ePointList,
  vk::PrimitiveTopology::eLineList,
  vk::PrimitiveTopology::eTriangleList,
};

void ezGALRenderCommandEncoderVulkan::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_currentPrimitiveTopology = GALTopologyToVulkan[Topology];
  m_bPipelineStateDirty = true;
}

void ezGALRenderCommandEncoderVulkan::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  m_pCurrentBlendState = pBlendState != nullptr ? static_cast<const ezGALBlendStateVulkan*>(pBlendState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALRenderCommandEncoderVulkan::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pCurrentDepthStencilState = pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateVulkan*>(pDepthStencilState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALRenderCommandEncoderVulkan::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pCurrentRasterizerState = pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateVulkan*>(pRasterizerState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALRenderCommandEncoderVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  vk::Viewport viewport = {};
  viewport.x = rect.x;
  viewport.y = rect.y;
  viewport.width = rect.width;
  viewport.height = rect.height;
  viewport.minDepth = fMinDepth;
  viewport.maxDepth = fMaxDepth;

  m_pCommandBuffer->setViewport(0, 1, &viewport);
}

void ezGALRenderCommandEncoderVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  vk::Rect2D scissor;
  scissor.offset.x = rect.x;
  scissor.offset.y = rect.y;
  scissor.extent.width = rect.width;
  scissor.extent.height = rect.height;

  m_pCommandBuffer->setScissor(0, 1, &scissor);
}

void ezGALRenderCommandEncoderVulkan::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}
