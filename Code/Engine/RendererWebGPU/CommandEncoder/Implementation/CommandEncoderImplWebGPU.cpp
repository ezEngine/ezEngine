#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererWebGPU/CommandEncoder/CommandEncoderImplWebGPU.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/BufferWebGPU.h>
#include <RendererWebGPU/Resources/RenderTargetViewWebGPU.h>
#include <RendererWebGPU/Resources/ResourceViewWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>
#include <RendererWebGPU/Resources/UnorderedAccessViewWebGPU.h>
#include <RendererWebGPU/Shader/ShaderWebGPU.h>
#include <RendererWebGPU/Shader/VertexDeclarationWebGPU.h>
#include <RendererWebGPU/State/StateWebGPU.h>

ezGALCommandEncoderImplWebGPU::ezGALCommandEncoderImplWebGPU(ezGALDeviceWebGPU& ref_deviceWebGPU)
  : m_GALDeviceWebGPU(ref_deviceWebGPU)
{
  EZ_WEBGPU_TRACE();

  m_Encoder = m_GALDeviceWebGPU.GetDevice().CreateCommandEncoder();
}

ezGALCommandEncoderImplWebGPU::~ezGALCommandEncoderImplWebGPU()
{
  EZ_WEBGPU_TRACE();

  m_Encoder = nullptr;
}

void ezGALCommandEncoderImplWebGPU::SetShaderPlatform(const ezGALShader* pShader0)
{
  // EZ_WEBGPU_TRACE();

  m_BindGroupEntries.Clear();

  const ezGALShaderWebGPU* pShader = (const ezGALShaderWebGPU*)pShader0;

  if (!pShader->m_bLoadedSuccessfully)
  {
    m_PipelineDesc.fragment = nullptr;
    return;
  }

  m_PipelineDesc.vertex.module = pShader->m_ShaderModuleVS;

  if (pShader->m_ShaderModuleFS == nullptr)
  {
    m_PipelineDesc.fragment = nullptr;
  }
  else
  {
    m_PipelineDesc.fragment = &m_FragmentState;

    m_FragmentState.module = pShader->m_ShaderModuleFS;
  }
}

void ezGALCommandEncoderImplWebGPU::SetConstantBufferPlatform(const ezShaderResourceBinding& binding, const ezGALBuffer* pBuffer0)
{
  EZ_WEBGPU_TRACE_TEXT("Constant Buffer");

  EZ_ASSERT_DEBUG(binding.m_iSlot >= 0, "Invalid slot index.");

  const ezGALBufferWebGPU* pBuffer = (const ezGALBufferWebGPU*)pBuffer0;

  for (auto& e : m_BindGroupEntries)
  {
    if ((ezInt32)e.binding == (ezInt32)binding.m_iSlot)
    {
      e.textureView = nullptr;
      e.sampler = nullptr;
      e.buffer = pBuffer->m_Buffer;
      return;
    }
  }

  auto& e = m_BindGroupEntries.ExpandAndGetRef();
  e.binding = (ezUInt32)binding.m_iSlot;
  e.buffer = pBuffer->m_Buffer;
}

void ezGALCommandEncoderImplWebGPU::SetSamplerStatePlatform(const ezShaderResourceBinding& binding, const ezGALSamplerState* pSampler0)
{
  EZ_WEBGPU_TRACE_TEXT("Sampler State");

  const ezGALSamplerStateWebGPU* pSampler = (const ezGALSamplerStateWebGPU*)pSampler0;

  for (auto& e : m_BindGroupEntries)
  {
    if ((ezInt32)e.binding == (ezInt32)binding.m_iSlot)
    {
      e.buffer = nullptr;
      e.textureView = nullptr;
      e.sampler = pSampler->m_Sampler;
      return;
    }
  }

  auto& e = m_BindGroupEntries.ExpandAndGetRef();
  e.binding = (ezUInt32)binding.m_iSlot;
  e.sampler = pSampler->m_Sampler;
}

void ezGALCommandEncoderImplWebGPU::SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureResourceView* pResourceView0)
{
  EZ_WEBGPU_TRACE_TEXT("Texture Binding");

  const ezGALTextureResourceViewWebGPU* pTextureView = (const ezGALTextureResourceViewWebGPU*)pResourceView0;

  for (auto& e : m_BindGroupEntries)
  {
    if ((ezInt32)e.binding == (ezInt32)binding.m_iSlot)
    {
      e.buffer = nullptr;
      e.textureView = pTextureView->m_TextureView;
      e.sampler = nullptr;
      return;
    }
  }

  auto& e = m_BindGroupEntries.ExpandAndGetRef();
  e.binding = (ezUInt32)binding.m_iSlot;
  e.textureView = pTextureView->m_TextureView;
}

void ezGALCommandEncoderImplWebGPU::SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferResourceView* pResourceView)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureUnorderedAccessView* pUnorderedAccessView)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferUnorderedAccessView* pUnorderedAccessView)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data)
{
  EZ_WEBGPU_TRACE();
}

// Query functions

ezGALTimestampHandle ezGALCommandEncoderImplWebGPU::InsertTimestampPlatform()
{
  EZ_WEBGPU_TRACE();
  return {};
}

ezGALOcclusionHandle ezGALCommandEncoderImplWebGPU::BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type)
{
  EZ_WEBGPU_TRACE();
  return {};
}

void ezGALCommandEncoderImplWebGPU::EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion)
{
  EZ_WEBGPU_TRACE();
}

ezGALFenceHandle ezGALCommandEncoderImplWebGPU::InsertFencePlatform()
{
  EZ_WEBGPU_TRACE();
  return {};
}

// Resource update functions

void ezGALCommandEncoderImplWebGPU::ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4 vClearValues)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4 vClearValues)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4U32 vClearValues)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4U32 vClearValues)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode)
{
  // EZ_WEBGPU_TRACE();

  const ezGALBufferWebGPU* pRealBuffer = (const ezGALBufferWebGPU*)pDestination;
  m_GALDeviceWebGPU.GetDevice().GetQueue().WriteBuffer(pRealBuffer->m_Buffer, uiDestOffset, sourceData.GetPtr(), sourceData.GetCount());
}

void ezGALCommandEncoderImplWebGPU::CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezVec3U32& vDestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource,
  const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ReadbackTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::GenerateMipMapsPlatform(const ezGALTextureResourceView* pResourceView)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::FlushPlatform()
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::PushMarkerPlatform(const char* szMarker)
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::PopMarkerPlatform()
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::InsertEventMarkerPlatform(const char* szMarker)
{
  EZ_WEBGPU_TRACE();
}

//////////////////////////////////////////////////////////////////////////

void ezGALCommandEncoderImplWebGPU::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup)
{
  EZ_WEBGPU_TRACE();

  m_BindGroupEntries.Clear();

  EZ_ASSERT_DEV(m_ActivePass == nullptr, "Render pass is already active");

  // TODO WebGPU: use renderingSetup
  // ezGALRenderTargetViewWebGPU* pRtView = (ezGALRenderTargetViewWebGPU*)m_GALDeviceWebGPU.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(0));

  wgpu::SurfaceTexture surfaceTexture;
  m_GALDeviceWebGPU.m_MainSurface.GetCurrentTexture(&surfaceTexture);

  static float r = 0.0f;
  static float g = 0.1f;
  static float b = 0.2f;
  r = ezMath::WrapFloat01(r += 0.003f);
  g = ezMath::WrapFloat01(g += 0.0021f);
  b = ezMath::WrapFloat01(b += 0.007f);

  wgpu::RenderPassColorAttachment attachment;
  attachment.view = surfaceTexture.texture.CreateView();
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearValue.r = r;
  attachment.clearValue.g = g;
  attachment.clearValue.b = b;

  wgpu::RenderPassDescriptor renderpass;
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  m_Encoder = m_GALDeviceWebGPU.GetDevice().CreateCommandEncoder(); // TODO WebGPU: have to recreate the command encoder every time ???
  m_ActivePass = m_Encoder.BeginRenderPass(&renderpass);
}

void ezGALCommandEncoderImplWebGPU::EndRenderingPlatform()
{
  EZ_WEBGPU_TRACE();

  m_ActivePass.End();
  wgpu::CommandBuffer commands = m_Encoder.Finish();

  m_GALDeviceWebGPU.GetDevice().GetQueue().Submit(1, &commands);

  m_ActivePass = nullptr;
}

void ezGALCommandEncoderImplWebGPU::BeginComputePlatform()
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::EndComputePlatform()
{
  EZ_WEBGPU_TRACE();
}

void ezGALCommandEncoderImplWebGPU::ClearPlatform(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  EZ_WEBGPU_TRACE();
}

ezResult ezGALCommandEncoderImplWebGPU::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  EZ_WEBGPU_TRACE();

  wgpu::ColorTargetState colorTargetState;
  colorTargetState.format = wgpu::TextureFormat::BGRA8Unorm; // TODO WebGPU

  // TODO WebPGU: get colorTargetState.writeMask = from blend state + set blend state

  m_FragmentState.targetCount = 1;
  m_FragmentState.targets = &colorTargetState;

  // TODO WebGPU: create these layouts
  // wgpu::PipelineLayoutDescriptor pd;
  // pd.bindGroupLayoutCount =
  // pd.bindGroupLayouts =
  // m_GALDeviceWebGPU.GetDevice().CreatePipelineLayout(
  // m_GALDeviceWebGPU.GetDevice().CreateBindGroupLayout
  // m_GALDeviceWebGPU.GetDevice().CreatePipelineLayout
  // m_PipelineDesc.layout

  auto pipeline = m_GALDeviceWebGPU.GetDevice().CreateRenderPipeline(&m_PipelineDesc);

  {
    // TODO WebGPU: bind constant buffers by actual location

    wgpu::BindGroupDescriptor desc;
    desc.layout = pipeline.GetBindGroupLayout(0);
    desc.entryCount = m_BindGroupEntries.GetCount();
    desc.entries = m_BindGroupEntries.GetData();

    auto uniformBindGroup = m_GALDeviceWebGPU.GetDevice().CreateBindGroup(&desc);

    m_ActivePass.SetBindGroup(0, uniformBindGroup);
  }

  m_ActivePass.SetPipeline(pipeline);
  m_ActivePass.DrawIndexed(uiIndexCount, 1, uiStartIndex);

  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

void ezGALCommandEncoderImplWebGPU::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer0)
{
  // EZ_WEBGPU_TRACE();

  ezGALBufferWebGPU* pIndexBuffer = (ezGALBufferWebGPU*)pIndexBuffer0;
  const bool b16Bit = pIndexBuffer->GetDescription().m_uiStructSize == 2;
  m_ActivePass.SetIndexBuffer(pIndexBuffer->m_Buffer, b16Bit ? wgpu::IndexFormat::Uint16 : wgpu::IndexFormat::Uint32);
}

void ezGALCommandEncoderImplWebGPU::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  // EZ_WEBGPU_TRACE();

  // TODO WebGPU: vertex buffer slot ?

  ezGALBufferWebGPU* pRealBuffer = (ezGALBufferWebGPU*)pVertexBuffer;
  m_ActivePass.SetVertexBuffer(uiSlot, pRealBuffer->m_Buffer);
}

void ezGALCommandEncoderImplWebGPU::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration0)
{
  // EZ_WEBGPU_TRACE();

  // TODO WebGPU: more than one vertex buffer

  ezGALVertexDeclarationWebGPU* pVertexDeclaration = (ezGALVertexDeclarationWebGPU*)pVertexDeclaration0;
  m_PipelineDesc.vertex.bufferCount = 1;
  m_PipelineDesc.vertex.buffers = &pVertexDeclaration->m_Layout;
}

void ezGALCommandEncoderImplWebGPU::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum topology)
{
  switch (topology)
  {
    case ezGALPrimitiveTopology::Points:
      m_PipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;
      break;
    case ezGALPrimitiveTopology::Lines:
      m_PipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineList;
      break;
    case ezGALPrimitiveTopology::Triangles:
      m_PipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void ezGALCommandEncoderImplWebGPU::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& blendFactor, ezUInt32 uiSampleMask)
{
  EZ_WEBGPU_TRACE();

  // TODO WebGPU: Blend State

  // m_PipelineDesc.fragment->targets[0].blend =
  // uiSampleMask ?

  wgpu::Color c;
  c.r = blendFactor.r;
  c.g = blendFactor.g;
  c.b = blendFactor.b;
  c.a = blendFactor.a;
  m_ActivePass.SetBlendConstant(&c);
}

void ezGALCommandEncoderImplWebGPU::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  EZ_WEBGPU_TRACE();

  // TODO WebGPU: Depth Stencil State

  // m_PipelineDesc.depthStencil
}

void ezGALCommandEncoderImplWebGPU::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  // EZ_WEBGPU_TRACE();

  const auto& desc = pRasterizerState->GetDescription();

  m_PipelineDesc.primitive.frontFace = desc.m_bFrontCounterClockwise ? wgpu::FrontFace::CCW : wgpu::FrontFace::CW;

  switch (desc.m_CullMode)
  {
    case ezGALCullMode::Back:
      m_PipelineDesc.primitive.cullMode = wgpu::CullMode::Back;
      break;
    case ezGALCullMode::Front:
      m_PipelineDesc.primitive.cullMode = wgpu::CullMode::Front;
      break;
    case ezGALCullMode::None:
      m_PipelineDesc.primitive.cullMode = wgpu::CullMode::None;
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  // TODO WebGPU: how to enable and disable scissor test? And other features.
  EZ_ASSERT_DEBUG(pRasterizerState->GetDescription().m_iDepthBias == 0, "Depth bias not implemented.");
  EZ_ASSERT_DEBUG(pRasterizerState->GetDescription().m_fDepthBiasClamp == 0.0f, "Depth bias not implemented.");
  EZ_ASSERT_DEBUG(pRasterizerState->GetDescription().m_fSlopeScaledDepthBias == 0.0f, "Depth bias not implemented.");
  EZ_ASSERT_DEBUG(!pRasterizerState->GetDescription().m_bWireFrame, "WebGPU does not support wireframe mode.");
  EZ_ASSERT_DEBUG(!pRasterizerState->GetDescription().m_bScissorTest, "Scissor test not implemented.");
  EZ_ASSERT_DEBUG(!pRasterizerState->GetDescription().m_bConservativeRasterization, "Conservative rasterization not implemented or not supported by WebGPU.");
}

void ezGALCommandEncoderImplWebGPU::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  m_ActivePass.SetViewport(rect.x, rect.y, rect.width, rect.height, fMinDepth, fMaxDepth);
}

void ezGALCommandEncoderImplWebGPU::SetScissorRectPlatform(const ezRectU32& rect)
{
  m_ActivePass.SetScissorRect(rect.x, rect.y, rect.width, rect.height);
}

ezResult ezGALCommandEncoderImplWebGPU::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALCommandEncoderImplWebGPU::DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}
