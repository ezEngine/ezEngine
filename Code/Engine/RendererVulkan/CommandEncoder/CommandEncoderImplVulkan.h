
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALBlendStateVulkan;
class ezGALBufferVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALRasterizerStateVulkan;
class ezGALResourceViewVulkan;
class ezGALSamplerStateVulkan;
class ezGALShaderVulkan;
class ezGALUnorderedAccessViewVulkan;
class ezGALDeviceVulkan;

class EZ_RENDERERVULKAN_DLL ezGALCommandEncoderImplVulkan : public ezGALCommandEncoderCommonPlatformInterface, public ezGALCommandEncoderRenderPlatformInterface, public ezGALCommandEncoderComputePlatformInterface
{
public:
  ezGALCommandEncoderImplVulkan(ezGALDeviceVulkan& device);
  ~ezGALCommandEncoderImplVulkan();

  void Reset();
  void MarkDirty();

  // ezGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void BeginQueryPlatform(const ezGALQuery* pQuery) override;
  virtual void EndQueryPlatform(const ezGALQuery* pQuery) override;
  virtual ezResult GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(ezGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const ezGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, ezArrayPtr<ezGALTextureSubresource> SourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> TargetData) override;

  virtual void GenerateMipMapsPlatform(const ezGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  // ezGALCommandEncoderRenderPlatformInterface
  void BeginRendering(vk::CommandBuffer& commandBuffer, const ezGALRenderingSetup& renderingSetup);
  void EndRendering();

  // Draw functions

  virtual void ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) override;

  virtual void DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;
  virtual void EndStreamOutPlatform() override;

  // State functions

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer) override;
  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset) override;


  // ezGALCommandEncoderComputePlatformInterface
  // Dispatch
  void BeginCompute(vk::CommandBuffer& commandBuffer);
  void EndCompute();

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

private:
  void FlushDeferredStateChanges();

  ezGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device m_vkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;

  // Cache flags.
  bool m_bPipelineStateDirty = true;
  bool m_bViewportDirty = true;
  bool m_bIndexBufferDirty = false;
  bool m_bDescriptorsDirty = false;
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bRenderPassActive = false;

  // Bound objects for deferred state flushes
  ezResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  ezResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Framebuffer m_frameBuffer;
  vk::RenderPassBeginInfo m_renderPass;
  ezHybridArray<vk::ClearValue, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;

  vk::Viewport m_viewport;
  vk::Rect2D m_scissor;
  bool m_bScissorEnabled = false;

  const ezGALRenderTargetView* m_pBoundRenderTargets[EZ_GAL_MAX_RENDERTARGET_COUNT] = {};
  const ezGALRenderTargetView* m_pBoundDepthStencilTarget = nullptr;
  ezUInt32 m_uiBoundRenderTargetCount;

  const ezGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer m_pBoundVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  const ezGALBufferVulkan* m_pBoundConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  ezHybridArray<const ezGALResourceViewVulkan*, 16> m_pBoundShaderResourceViews[ezGALShaderStage::ENUM_COUNT] = {};
  ezHybridArray<const ezGALUnorderedAccessViewVulkan*, 16> m_pBoundUnoderedAccessViews;
  const ezGALSamplerStateVulkan* m_pBoundSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SAMPLER_COUNT] = {};
};
