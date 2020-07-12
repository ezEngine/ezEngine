
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

  currentCmdBuffer.drawIndexedIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVulkanBuffer(), uiArgumentOffsetInBytes, 1, 0);
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

  currentCmdBuffer.drawIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVulkanBuffer(), uiArgumentOffsetInBytes, 1, 0);
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

  currentCmdBuffer.dispatchIndirect(static_cast<const ezGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVulkanBuffer(), uiArgumentOffsetInBytes);
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
    currentCmdBuffer.bindIndexBuffer(pVulkanBuffer->GetVulkanBuffer(), 0, pVulkanBuffer->GetIndexFormat());
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

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pVertexBuffer)->GetVulkanBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALContextVulkan::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  if (pVertexDeclaration)
  {
    m_pCurrentVertexLayout = &static_cast<const ezGALVertexDeclarationVulkan*>(pVertexDeclaration)->GetInputLayout();
  }
  // TODO deferr to pipeline update
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
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const ezGALBufferVulkan*>(pBuffer)->GetVulkanBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);

  m_bPipelineStateDirty = true;
}

void ezGALContextVulkan::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const ezGALSamplerStateVulkan*>(pSamplerState)->GetSamplerState() : nullptr;
  m_BoundSamplerStatesRange[Stage].SetToIncludeValue(uiSlot);
}

void ezGALContextVulkan::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const ezGALResourceViewVulkan*>(pResourceView)->GetResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[Stage].SetToIncludeValue(uiSlot);
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
        m_pBoundRenderTargets[i] = static_cast<const ezGALRenderTargetViewVulkan*>(pRenderTargetViews[i])->GetRenderTargetView();
      }
    }

    if (pDepthStencilView != nullptr)
    {
      m_pBoundDepthStencilTarget = static_cast<const ezGALRenderTargetViewVulkan*>(pDepthStencilView)->GetDepthStencilView();
    }

    // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
    m_pDXContext->OMSetRenderTargets(ezMath::Max(pRenderTargetViews.GetCount(), m_uiBoundRenderTargetCount), m_pBoundRenderTargets,
      m_pBoundDepthStencilTarget);

    m_uiBoundRenderTargetCount = pRenderTargetViews.GetCount();
  }
  else
  {
    m_pBoundDepthStencilTarget = nullptr;
    m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_uiBoundRenderTargetCount = 0;
  }
}

void ezGALContextVulkan::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  m_pBoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

void ezGALContextVulkan::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pDXContext->OMSetBlendState(pBlendState != nullptr ? static_cast<const ezGALBlendStateVulkan*>(pBlendState)->GetDXBlendState() : nullptr,
    BlendFactors, uiSampleMask);
}

void ezGALContextVulkan::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(pDepthStencilState != nullptr
                                         ? static_cast<const ezGALDepthStencilStateVulkan*>(pDepthStencilState)->GetDXDepthStencilState()
                                         : nullptr,
    uiStencilRefValue);
}

void ezGALContextVulkan::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(
    pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateVulkan*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void ezGALContextVulkan::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void ezGALContextVulkan::SetScissorRectPlatform(const ezRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void ezGALContextVulkan::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Fence & Query functions

void ezGALContextVulkan::InsertFencePlatform(const ezGALFence* pFence)
{
  m_pDXContext->End(static_cast<const ezGALFenceVulkan*>(pFence)->GetDXFence());
}

bool ezGALContextVulkan::IsFenceReachedPlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  if (m_pDXContext->GetData(static_cast<const ezGALFenceVulkan*>(pFence)->GetDXFence(), &data, sizeof(data), 0) == S_OK)
  {
    EZ_ASSERT_DEV(data == TRUE, "Implementation error");
    return true;
  }

  return false;
}

void ezGALContextVulkan::WaitForFencePlatform(const ezGALFence* pFence)
{
  BOOL data = FALSE;
  while (m_pDXContext->GetData(static_cast<const ezGALFenceVulkan*>(pFence)->GetDXFence(), &data, sizeof(data), 0) != S_OK)
  {
    ezThreadUtils::YieldTimeSlice();
  }

  EZ_ASSERT_DEV(data == TRUE, "Implementation error");
}

void ezGALContextVulkan::BeginQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const ezGALQueryVulkan*>(pQuery)->GetDXQuery());
}

void ezGALContextVulkan::EndQueryPlatform(const ezGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const ezGALQueryVulkan*>(pQuery)->GetDXQuery());
}

ezResult ezGALContextVulkan::GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult)
{
  return m_pDXContext->GetData(static_cast<const ezGALQueryVulkan*>(pQuery)->GetDXQuery(), &uiQueryResult, sizeof(ezUInt64),
           D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE
           ? EZ_FAILURE
           : EZ_SUCCESS;
}

void ezGALContextVulkan::InsertTimestampPlatform(ezGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = static_cast<ezGALDeviceVulkan*>(GetDevice())->GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

void ezGALContextVulkan::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferVulkan*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void ezGALContextVulkan::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource,
  ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const ezGALBufferVulkan*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void ezGALContextVulkan::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
  ezGALUpdateMode::Enum updateMode)
{
  EZ_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  ID3D11Buffer* pDXDestination = static_cast<const ezGALBufferVulkan*>(pDestination)->GetDXBuffer();

  if (pDestination->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    EZ_ASSERT_DEV(uiDestOffset == 0 && pSourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for Vulkan.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    if (updateMode == ezGALUpdateMode::CopyToTempStorage)
    {
      if (ID3D11Resource* pDXTempBuffer = static_cast<ezGALDeviceVulkan*>(GetDevice())->FindTempBuffer(pSourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

        memcpy(MapResult.pData, pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, pSourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        EZ_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = (updateMode == ezGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(ezMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
    }
  }
}

void ezGALContextVulkan::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureVulkan*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureVulkan*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void ezGALContextVulkan::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezVec3U32& DestinationPoint, const ezGALTexture* pSource,
  const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureVulkan*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureVulkan*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice,
    pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource =
    D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {Box.m_vMin.x, Box.m_vMin.y, Box.m_vMin.z, Box.m_vMax.x, Box.m_vMax.y, Box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, DestinationPoint.x, DestinationPoint.y, DestinationPoint.z, pDXSource,
    srcSubResource, &srcBox);
}

void ezGALContextVulkan::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureVulkan*>(pDestination)->GetDXTexture();

  ezUInt32 uiWidth = ezMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  ezUInt32 uiHeight = ezMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  ezUInt32 uiDepth = ezMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);
  ezGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = static_cast<ezGALDeviceVulkan*>(GetDevice())->FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    EZ_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

    ezUInt32 uiRowPitch = uiWidth * ezGALResourceFormat::GetBitsPerElement(format) / 8;
    ezUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    EZ_ASSERT_DEV(pSourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, pSourceData.m_uiRowPitch);
    EZ_ASSERT_DEV(pSourceData.m_uiSlicePitch == 0 || pSourceData.m_uiSlicePitch == uiSlicePitch,
      "Invalid slice pitch. Expected {0} got {1}", uiSlicePitch, pSourceData.m_uiSlicePitch);

    memcpy(MapResult.pData, pSourceData.m_pData, uiSlicePitch * uiDepth);

    m_pDXContext->Unmap(pDXTempTexture, 0);

    ezUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice,
      pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, DestinationBox.m_vMin.x, DestinationBox.m_vMin.y,
      DestinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    EZ_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void ezGALContextVulkan::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const ezGALTextureVulkan*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const ezGALTextureVulkan*>(pSource)->GetDXTexture();

  ezUInt32 dstSubResource = D3D11CalcSubresource(DestinationSubResource.m_uiMipLevel, DestinationSubResource.m_uiArraySlice,
    pDestination->GetDescription().m_uiMipLevelCount);
  ezUInt32 srcSubResource =
    D3D11CalcSubresource(SourceSubResource.m_uiMipLevel, SourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = static_cast<ezGALDeviceVulkan*>(GetDevice())
                           ->GetFormatLookupTable()
                           .GetFormatInfo(pDestination->GetDescription().m_Format)
                           .m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void ezGALContextVulkan::ReadbackTexturePlatform(const ezGALTexture* pTexture)
{
  const ezGALTextureVulkan* pDXTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  EZ_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
  }
}

void ezGALContextVulkan::CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture,
  const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  const ezGALTextureVulkan* pDXTexture = static_cast<const ezGALTextureVulkan*>(pTexture);

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");

  D3D11_MAPPED_SUBRESOURCE Mapped;
  if (SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), 0, D3D11_MAP_READ, 0, &Mapped)))
  {
    if (Mapped.RowPitch == (*pData)[0].m_uiRowPitch)
    {
      const ezUInt32 uiMemorySize = ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) *
                                    pDXTexture->GetDescription().m_uiWidth * pDXTexture->GetDescription().m_uiHeight / 8;
      memcpy((*pData)[0].m_pData, Mapped.pData, uiMemorySize);
    }
    else
    {
      // Copy row by row
      for (ezUInt32 y = 0; y < pDXTexture->GetDescription().m_uiHeight; ++y)
      {
        const void* pSource = ezMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
        void* pDest = ezMemoryUtils::AddByteOffset((*pData)[0].m_pData, y * (*pData)[0].m_uiRowPitch);

        memcpy(pDest, pSource,
          ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * pDXTexture->GetDescription().m_uiWidth / 8);
      }
    }

    m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), 0);
  }
}

void ezGALContextVulkan::GenerateMipMapsPlatform(const ezGALResourceView* pResourceView)
{
  const ezGALResourceViewVulkan* pDXResourceView = static_cast<const ezGALResourceViewVulkan*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void ezGALContextVulkan::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void ezGALContextVulkan::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void ezGALContextVulkan::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void ezGALContextVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Context_Implementation_ContextVulkan);
