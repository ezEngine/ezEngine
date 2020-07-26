
#include <RendererVulkanPCH.h>

#include <RendererVulkan/Context/ContextVulkan.h>
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


ezGALContextVulkan::ezGALContextVulkan(ezGALDevice* pDevice)
  : ezGALContext(pDevice)
{
  EZ_ASSERT_RELEASE(m_device, "Invalid vulkan context!");

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  m_device = pVulkanDevice->GetVulkanDevice();

  vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
  commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreateInfo.queueFamilyIndex = pVulkanDevice->GetQueueFamilyIndices()[0]; // TODO what to do if there is more than one index

  m_commandPool = m_device.createCommandPool(commandPoolCreateInfo);

  vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
  commandBufferAllocateInfo.commandBufferCount = NUM_CMD_BUFFERS;
  commandBufferAllocateInfo.commandPool = m_commandPool;
  commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;

  m_device.allocateCommandBuffers(&commandBufferAllocateInfo, m_commandBuffers);

  vk::FenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
  for (ezUInt32 i = 0; i < NUM_CMD_BUFFERS; ++i)
  {
    m_commandBufferFences[i] = m_device.createFence(fenceCreateInfo);
  }
}

ezGALContextVulkan::~ezGALContextVulkan()
{
  m_device.waitForFences(vk::ArrayProxy<const vk::Fence>(NUM_CMD_BUFFERS, m_commandBufferFences), VK_TRUE, 1000000000ui64);

  for (ezUInt32 i = 0; i < NUM_CMD_BUFFERS; ++i)
  {
    m_device.destroyFence(m_commandBufferFences[i]);
    m_commandBufferFences[i] = nullptr;
  }

  m_device.freeCommandBuffers(m_commandPool, NUM_CMD_BUFFERS, m_commandBuffers);
  m_device.destroyCommandPool(m_commandPool);
}


// Draw functions

void ezGALContextVulkan::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil,
                                     float fDepthClear, ezUInt8 uiStencilClear)
{
  // TODO
  // * start appropriate render pass with respective framebuffer of all the bound attachments
  // * clear execute a clear command on the recording command buffer
}

void ezGALContextVulkan::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues)
{
  // this looks to require custom code, either using buffer copies or
  // clearing via a compute shader
}

void ezGALContextVulkan::ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues)
{
  // Same as the other clearing variant
}

void ezGALContextVulkan::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  // TODO make sure we are inside a render pass and everything is bound as required
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.draw(uiVertexCount, 1, uiStartVertex, 0);
}

void ezGALContextVulkan::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

#if 0//EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  currentCmdBuffer.drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);

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
  currentCmdBuffer.drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
#endif
}

void ezGALContextVulkan::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  //FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALContextVulkan::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALContextVulkan::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  //FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALContextVulkan::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.drawIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void ezGALContextVulkan::DrawAutoPlatform()
{
  //FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextVulkan::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();
}

void ezGALContextVulkan::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}
#if 0
static void SetShaderResources(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(ezGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, ezUInt32 uiStartSlot, ezUInt32 uiNumSlots,
  ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case ezGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case ezGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
#endif

#if 0

// Some state changes are deferred so they can be updated faster
void ezGALContextVulkan::FlushDeferredStateChanges()
{
  if (m_BoundVertexBuffersRange.IsValid())
  {
    // TODO vertex buffer needs to be in index buffer resource state
    const ezUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot,
      m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_pBoundUnoderedAccessViewsRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_pBoundUnoderedAccessViewsRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_pBoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_pBoundUnoderedAccessViews.GetData() + uiStartSlot,
      nullptr); // Todo: Count reset.

    m_pBoundUnoderedAccessViewsRange.Reset();
  }

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots,
        m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const ezUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const ezUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((ezGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
}

#endif

// Dispatch

void ezGALContextVulkan::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALContextVulkan::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  //FlushDeferredStateChanges();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.dispatchIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetBuffer(), uiArgumentOffsetInBytes);
}


// State setting functions

void ezGALContextVulkan::SetShaderPlatform(const ezGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_pCurrentShader = static_cast<const ezGALShaderVulkan*>(pShader);
    m_bPipelineStateDirty = true;
  }
}

void ezGALContextVulkan::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  // TODO index buffer needs to be in index buffer resource state
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  if (pIndexBuffer != nullptr)
  {
    const ezGALBufferVulkan* pVulkanBuffer = static_cast<const ezGALBufferVulkan*>(pIndexBuffer);
    currentCmdBuffer.bindIndexBuffer(pVulkanBuffer->GetBuffer(), 0, pVulkanBuffer->GetIndexType());
  }
  else
  {
    // TODO binding a null buffer is not allowed in vulkan
    EZ_ASSERT_NOT_IMPLEMENTED;
    //currentCmdBuffer.bindIndexBuffer({}, 0, vk::IndexType::eUint16);
  }
}

void ezGALContextVulkan::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALContextVulkan::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
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

void ezGALContextVulkan::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_currentPrimitiveTopology = GALTopologyToVulkan[Topology];
  m_bPipelineStateDirty = true;
}

void ezGALContextVulkan::SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer)
{
  // \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pBuffer)->GetBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALContextVulkan::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const ezGALSamplerStateVulkan*>(pSamplerState)->GetSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALContextVulkan::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const ezGALResourceViewVulkan*>(pResourceView) : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALContextVulkan::SetRenderTargetSetupPlatform(ezArrayPtr<const ezGALRenderTargetView*> pRenderTargetViews,
  const ezGALRenderTargetView* pDepthStencilView)
{
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;

  if (!pRenderTargetViews.IsEmpty() || pDepthStencilView != nullptr)
  {
    for (ezUInt32 i = 0; i < pRenderTargetViews.GetCount(); i++)
    {
      if (pRenderTargetViews[i] != nullptr)
      {
        m_pBoundRenderTargets[i] = static_cast<const ezGALRenderTargetViewVulkan*>(pRenderTargetViews[i]);
      }
    }

    if (pDepthStencilView != nullptr)
    {
      m_pBoundDepthStencilTarget = static_cast<const ezGALRenderTargetViewVulkan*>(pDepthStencilView);
    }

    // TODO start renderpass, bind framebuffer and so on deferred
    m_bFrameBufferDirty = true;
    m_uiBoundRenderTargetCount = pRenderTargetViews.GetCount();
  }
  else
  {
    m_pBoundDepthStencilTarget = nullptr;
    // TODO stop renderpass?
    m_uiBoundRenderTargetCount = 0;
  }
}

void ezGALContextVulkan::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView) : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);

  m_bDescriptorsDirty = true;
}

void ezGALContextVulkan::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  m_pCurrentBlendState = pBlendState != nullptr ? static_cast<const ezGALBlendStateVulkan*>(pBlendState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALContextVulkan::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pCurrentDepthStencilState = pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateVulkan*>(pDepthStencilState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALContextVulkan::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pCurrentRasterizerState = pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateVulkan*>(pRasterizerState) : nullptr;
  m_bPipelineStateDirty = true;
}

void ezGALContextVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  vk::Viewport viewport = {};
  viewport.x = rect.x;
  viewport.y = rect.y;
  viewport.width = rect.width;
  viewport.height = rect.height;
  viewport.minDepth = fMinDepth;
  viewport.maxDepth = fMaxDepth;

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.setViewport(0, 1, &viewport);
}

void ezGALContextVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  vk::Rect2D scissor;
  scissor.offset.x = rect.x;
  scissor.offset.y = rect.y;
  scissor.extent.width = rect.width;
  scissor.extent.height = rect.height;

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.setScissor(0, 1, &scissor);
}

void ezGALContextVulkan::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Fence & Query functions

void ezGALContextVulkan::InsertFencePlatform(const ezGALFence* pFence)
{
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);

  vk::Queue queue = pVulkanDevice->GetQueue();
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  currentCmdBuffer.end();
  vk::SubmitInfo submitInfo = {};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &currentCmdBuffer;

  queue.submit(1, &submitInfo, pVulkanFence->GetFence());
}

bool ezGALContextVulkan::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());
  auto pVulkanFence = static_cast<const ezGALFenceVulkan*>(pFence);
  vk::Result fenceStatus = pVulkanDevice->GetVulkanDevice().getFenceStatus(pVulkanFence->GetFence());

  EZ_ASSERT_DEV(fenceStatus != vk::Result::eErrorDeviceLost, "Device lost during fence status query!");

  return fenceStatus == vk::Result::eSuccess;
}

void ezGALContextVulkan::WaitForFencePlatform(const ezGALFence* pFence)
{
  while (!IsFenceReachedPlatform(pFence))
  {
    ezThreadUtils::YieldTimeSlice();
  }
}

void ezGALContextVulkan::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  // TODO how to decide the query type etc in Vulkan?

  currentCmdBuffer.beginQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), {});
}

void ezGALContextVulkan::EndQueryPlatform(const ezGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.endQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID());
}

ezResult ezGALContextVulkan::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());
  auto pVulkanQuery = static_cast<const ezGALQueryVulkan*>(pQuery);
  vk::Result result = pVulkanDevice->GetVulkanDevice().getQueryPoolResults(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), 1u, sizeof(ezUInt64), &uiQueryResult, 0, vk::QueryResultFlagBits::e64);

  return result == vk::Result::eSuccess ? EZ_SUCCESS : EZ_FAILURE;
}

void ezGALContextVulkan::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  // TODO how to implement this in Vulkan?
  //ID3D11Query* pDXQuery = static_cast<ezGALDeviceVulkan*>(GetDevice())->GetTimestamp(hTimestamp);
  //
  //m_pDXContext->End(pDXQuery);
}

// Resource update functions

void ezGALContextVulkan::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  vk::Buffer destination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetBuffer();
  vk::Buffer source = static_cast<const ezGALBufferVulkan*>(pSource)->GetBuffer();

  EZ_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  // TODO do this in an immediate command buffer?
  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.copyBuffer(source, destination, 1, &bufferCopy);
}

void ezGALContextVulkan::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource,
  ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  vk::Buffer destination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetBuffer();
  vk::Buffer source = static_cast<const ezGALBufferVulkan*>(pSource)->GetBuffer();
  vk::BufferCopy bufferCopy = {};
  bufferCopy.dstOffset = uiDestOffset;
  bufferCopy.srcOffset = uiSourceOffset;
  bufferCopy.size = uiByteCount;

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.copyBuffer(source, destination, 1, &bufferCopy);
}

void ezGALContextVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
  ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pVulkanDestination = static_cast<const ezGALBufferVulkan*>(pDestination);

  if (pDestination->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
    currentCmdBuffer.updateBuffer(pVulkanDestination->GetBuffer(), uiDestOffset, pSourceData.GetCount(), pSourceData.GetPtr());
  }
  else
  {
    auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());

    if (updateMode == ezGALUpdateMode::CopyToTempStorage)
    {
      if (ezGALBufferVulkan* tmpBuffer = static_cast<ezGALDeviceVulkan*>(GetDevice())->FindTempBuffer(pSourceData.GetCount()))
      {
        EZ_ASSERT_DEV(tmpBuffer->GetSize() >= pSourceData.GetCount(), "Source data is too big to copy staged!");

        void* pData = pVulkanDevice->GetVulkanDevice().mapMemory(tmpBuffer->GetMemory(), tmpBuffer->GetMemoryOffset(), tmpBuffer->GetSize());

        EZ_ASSERT_DEV(pData, "Implementation error");

        ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

        pVulkanDevice->GetVulkanDevice().unmapMemory(tmpBuffer->GetMemory());

        CopyBufferRegionPlatform(pDestination, uiDestOffset, tmpBuffer, 0, pSourceData.GetCount());
      }
      else
      {
        EZ_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      // TODO is this behavior available on Vulkan?
      //D3D11_MAP mapType = (updateMode == ezGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      void* pData = pVulkanDevice->GetVulkanDevice().mapMemory(pVulkanDestination->GetMemory(), pVulkanDestination->GetMemoryOffset(), pVulkanDestination->GetSize());

      if (pData)
      {
        ezMemoryUtils::Copy((ezUInt8*)pData, pSourceData.GetPtr(), pSourceData.GetCount());

        pVulkanDevice->GetVulkanDevice().unmapMemory(pVulkanDestination->GetMemory());
      }
    }
  }
}

void ezGALContextVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto source = static_cast<const ezGALTextureVulkan*>(pSource);

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");
  EZ_ASSERT_DEBUG(destDesc.m_uiArraySize == srcDesc.m_uiArraySize, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiMipLevelCount == srcDesc.m_uiMipLevelCount, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiWidth == srcDesc.m_uiWidth, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiHeight == srcDesc.m_uiHeight, "");
  EZ_ASSERT_DEBUG(destDesc.m_uiDepth == srcDesc.m_uiDepth, "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  // TODO need to copy every mip level
  ezHybridArray<vk::ImageCopy, 14> imageCopies;

  for (int i = 0; i < destDesc.m_uiMipLevelCount; ++i)
  {
    vk::ImageCopy& imageCopy = imageCopies.ExpandAndGetRef();
    imageCopy.dstOffset = {};
    imageCopy.dstSubresource.aspectMask = imageAspect;
    imageCopy.dstSubresource.baseArrayLayer = 0;
    imageCopy.dstSubresource.layerCount = destDesc.m_uiArraySize;
    imageCopy.dstSubresource.mipLevel = i;
    imageCopy.extent.width = destDesc.m_uiWidth;
    imageCopy.extent.height = destDesc.m_uiHeight;
    imageCopy.extent.depth = destDesc.m_uiDepth;
    imageCopy.srcOffset = {};
    imageCopy.srcSubresource.aspectMask = imageAspect;
    imageCopy.srcSubresource.baseArrayLayer = 0;
    imageCopy.srcSubresource.layerCount = srcDesc.m_uiArraySize;
    imageCopy.srcSubresource.mipLevel = i;
  }

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.copyImage(source->GetImage(), vk::ImageLayout::eGeneral, destination->GetImage(), vk::ImageLayout::eGeneral, destDesc.m_uiMipLevelCount, imageCopies.GetData());
}

void ezGALContextVulkan::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezVec3U32& DestinationPoint, const ezGALTexture* pSource,
  const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  auto destination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto source = static_cast<const ezGALTextureVulkan*>(pSource);

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  ezVec3U32 extent = Box.m_vMax - Box.m_vMin;

  vk::ImageCopy imageCopy = {};
  imageCopy.dstOffset.x = DestinationPoint.x;
  imageCopy.dstOffset.y = DestinationPoint.y;
  imageCopy.dstOffset.z = DestinationPoint.z;
  imageCopy.dstSubresource.aspectMask = imageAspect;
  imageCopy.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.dstSubresource.layerCount = 1;
  imageCopy.dstSubresource.mipLevel = DestinationSubResource.m_uiMipLevel;
  imageCopy.extent.width = extent.x;
  imageCopy.extent.height = extent.y;
  imageCopy.extent.depth = extent.z;
  imageCopy.srcOffset.x = Box.m_vMin.x;
  imageCopy.srcOffset.y = Box.m_vMin.y;
  imageCopy.srcOffset.z = Box.m_vMin.z;
  imageCopy.srcSubresource.aspectMask = imageAspect;
  imageCopy.srcSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.srcSubresource.layerCount = 1;
  imageCopy.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.copyImage(source->GetImage(), vk::ImageLayout::eGeneral, destination->GetImage(), vk::ImageLayout::eGeneral, 1, &imageCopy);
}

void ezGALContextVulkan::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());

  if (ezGALTextureVulkan* pTempTexture = pVulkanDevice->FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    void* pData = pVulkanDevice->GetVulkanDevice().mapMemory(pTempTexture->GetMemory(), pTempTexture->GetMemoryOffset(), pTempTexture->GetMemorySize());
    EZ_ASSERT_DEV(pData, "Implementation error");

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch,
      "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, pSourceData.m_uiSlicePitch);

    ezMemoryUtils::Copy(pData, pSourceData.m_pData, uiSlicePitch * uiDepth);

    pVulkanDevice->GetVulkanDevice().unmapMemory(pTempTexture->GetMemory());

    ezGALTextureSubresource sourceSubResource;
    sourceSubResource.m_uiArraySlice = 0;
    sourceSubResource.m_uiMipLevel = 0;
    ezBoundingBoxu32 sourceBox = DestinationBox;
    sourceBox.Translate(-sourceBox.m_vMin);
    CopyTextureRegionPlatform(pDestination, DestinationSubResource, DestinationBox.m_vMin, pTempTexture, sourceSubResource, sourceBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void ezGALContextVulkan::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  EZ_ASSERT_ALWAYS(DestinationSubResource.m_uiMipLevel == 0, "Resolving of higher mips not implemented yet!");
  EZ_ASSERT_ALWAYS(SourceSubResource.m_uiMipLevel == 0, "Resolving of higher mips not implemented yet!");

  auto pVulkanDestination = static_cast<const ezGALTextureVulkan*>(pDestination);
  auto pVulkanSource = static_cast<const ezGALTextureVulkan*>(pSource);

  const ezGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const ezGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) == ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  // TODO need to determine size of the subresource
  vk::ImageResolve resolveRegion = {};
  resolveRegion.dstSubresource.aspectMask = imageAspect;
  resolveRegion.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.dstSubresource.layerCount = 1; // TODO is this correct?
  resolveRegion.dstSubresource.mipLevel = 0; // TODO implement resolve of higher mips
  resolveRegion.extent.width = destDesc.m_uiWidth;
  resolveRegion.extent.height = destDesc.m_uiHeight;
  resolveRegion.extent.depth = destDesc.m_uiDepth;
  resolveRegion.srcSubresource.aspectMask = imageAspect;
  resolveRegion.srcSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.srcSubresource.layerCount = 1;
  resolveRegion.srcSubresource.mipLevel = 0;// TODO implement resolve of higher mips

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.resolveImage(pVulkanSource->GetImage(), vk::ImageLayout::eGeneral, pVulkanDestination->GetImage, vk::ImageLayout::eGeneral, 1, &resolveRegion);
}

void ezGALContextVulkan::ReadbackTexturePlatform(const ezGALTexture* pTexture)
{
  const ezGALTextureVulkan* pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pVulkanTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

  EZ_ASSERT_DEV(pVulkanTexture->GetStagingBuffer(), "No staging resource available for read-back");
  EZ_ASSERT_DEV(pVulkanTexture->GetImage(), "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
  else
  {
    const ezGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();

    vk::ImageAspectFlagBits imageAspect = ezGALResourceFormat::IsDepthFormat(textureDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    vk::BufferImageCopy copyRegion = {};
    copyRegion.bufferImageHeight = textureDesc.m_uiHeight;
    copyRegion.bufferRowLength = textureDesc.m_uiWidth;
    copyRegion.bufferOffset = 0;
    copyRegion.imageExtent.width = textureDesc.m_uiWidth;
    copyRegion.imageExtent.height = textureDesc.m_uiWidth;
    copyRegion.imageExtent.depth = textureDesc.m_uiDepth;
    copyRegion.imageSubresource.aspectMask = imageAspect;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = textureDesc.m_uiArraySize;
    copyRegion.imageSubresource.mipLevel = 0; // TODO need to support all mip levels

    // TODO do we need to do this immediately?
    vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
    currentCmdBuffer.copyImageToBuffer(pVulkanTexture->GetImage(), vk::ImageLayout::eGeneral, pVulkanTexture->GetStagingBuffer()->GetBuffer(), 1, &copyRegion);
  }
}

void ezGALContextVulkan::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture,
  const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  auto pVulkanTexture = static_cast<const ezGALTextureVulkan*>(pTexture);
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(GetDevice());

  ezGALBufferVulkan* pStagingBuffer= pVulkanTexture->GetStagingBuffer();

  EZ_ASSERT_DEV(pStagingBuffer, "No staging resource available for read-back");

  // Data should be tightly packed in the staging buffer already, so
  // just map the memory and copy it over
  void* pSrcData = pVulkanDevice->GetVulkanDevice().mapMemory(pStagingBuffer->GetMemory(), pStagingBuffer->GetMemoryOffset(), pStagingBuffer->GetSize());
  if (pSrcData)
  {
    // TODO size of the buffer could missmatch the texture data size necessary
    ezMemoryUtils::Copy(pData->GetPtr()->m_pData, pSrcData, pStagingBuffer->GetSize());

    pVulkanDevice->GetVulkanDevice().unmapMemory(pStagingBuffer->GetMemory());
  }
}

void ezGALContextVulkan::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewVulkan* pDXResourceView = static_cast<const ezGALResourceViewVulkan*>(pResourceView);

  // TODO texture blit based approach
}

void ezGALContextVulkan::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void ezGALContextVulkan::PushMarkerPlatform(const char* szMarker)
{
  // TODO early out if device doesn't support debug markers

  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color, markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  currentCmdBuffer.debugMarkerBeginEXT(markerInfo);
}

void ezGALContextVulkan::PopMarkerPlatform()
{
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];
  currentCmdBuffer.debugMarkerEndEXT();
}

void ezGALContextVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  vk::CommandBuffer& currentCmdBuffer = m_commandBuffers[m_uiCurrentCmdBufferIndex];

  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugMarkerMarkerInfoEXT markerInfo = {};
  ezMemoryUtils::Copy(markerInfo.color, markerColor, EZ_ARRAY_SIZE(markerColor));
  markerInfo.pMarkerName = szMarker;
  currentCmdBuffer.debugMarkerInsertEXT(markerInfo);
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Context_Implementation_ContextVulkan);
