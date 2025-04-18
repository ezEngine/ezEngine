
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALBlendStateVulkan;
class ezGALBufferVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALRasterizerStateVulkan;
class ezGALTextureResourceViewVulkan;
class ezGALBufferResourceViewVulkan;
class ezGALSamplerStateVulkan;
class ezGALShaderVulkan;
class ezGALTextureUnorderedAccessViewVulkan;
class ezGALBufferUnorderedAccessViewVulkan;
class ezGALDeviceVulkan;
class ezFenceQueueVulkan;


class EZ_RENDERERVULKAN_DLL ezGALCommandEncoderImplVulkan : public ezGALCommandEncoderCommonPlatformInterface
{
public:
  ezGALCommandEncoderImplVulkan(ezGALDeviceVulkan& device);
  ~ezGALCommandEncoderImplVulkan();

  void Reset();

  void EndFrame();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, ezPipelineBarrierVulkan* pipelineBarrier);
  void BeforeCommandBufferSubmit();
  void AfterCommandBufferSubmit(vk::Fence submitFence);

  // ezGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(const ezShaderResourceBinding& binding, const ezGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(const ezShaderResourceBinding& binding, const ezGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureResourceView* pResourceView) override;
  virtual void SetResourceViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALTextureUnorderedAccessView* pUnorderedAccessView) override;
  virtual void SetUnorderedAccessViewPlatform(const ezShaderResourceBinding& binding, const ezGALBufferUnorderedAccessView* pUnorderedAccessView) override;
  virtual void SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data) override;

  // GPU -> CPU query functions

  virtual ezGALTimestampHandle InsertTimestampPlatform() override;
  virtual ezGALOcclusionHandle BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type) override;
  virtual void EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion) override;
  virtual ezGALFenceHandle InsertFencePlatform() override;


  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues) override;

  virtual void ClearUnorderedAccessViewPlatform(const ezGALTextureUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const ezGALBufferUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource) override;

  virtual void GenerateMipMapsPlatform(const ezGALTextureResourceView* pResourceView) override;

  void CopyImageToBuffer(const ezGALTextureVulkan* pSource, const ezGALBufferVulkan* pDestination);
  void CopyImageToBuffer(const ezGALTextureVulkan* pSource, vk::Buffer destination);

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  // ezGALCommandEncoderComputePlatformInterface
  // Dispatch
  virtual void BeginComputePlatform() override;
  virtual void EndComputePlatform() override;

  virtual ezResult DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;
  virtual ezResult DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;


  // ezGALCommandEncoderRenderPlatformInterface
  virtual void BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup) override;
  virtual void EndRenderingPlatform() override;

  // Draw functions

  virtual void ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) override;

  virtual ezResult DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) override;
  virtual ezResult DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) override;
  virtual ezResult DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) override;
  virtual ezResult DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;
  virtual ezResult DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) override;
  virtual ezResult DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  // State functions

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer, ezUInt32 uiOffset) override;
  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) override;



private:
  // Map resources from sets then slots to pointer.
  struct SetResources
  {
    ezDynamicArray<const ezGALBufferVulkan*> m_pBoundConstantBuffers;
    ezDynamicArray<const ezGALTextureResourceViewVulkan*> m_pBoundTextureResourceViews;
    ezDynamicArray<const ezGALBufferResourceViewVulkan*> m_pBoundBufferResourceViews;
    ezDynamicArray<const ezGALTextureUnorderedAccessViewVulkan*> m_pBoundTextureUnorderedAccessViews;
    ezDynamicArray<const ezGALBufferUnorderedAccessViewVulkan*> m_pBoundBufferUnorderedAccessViews;
    ezDynamicArray<const ezGALSamplerStateVulkan*> m_pBoundSamplerStates;
  };

private:
  ezResult FlushDeferredStateChanges();
  const ezGALTextureResourceViewVulkan* GetTextureResourceView(const SetResources& resources, const ezShaderResourceBinding& mapping);
  const ezGALBufferResourceViewVulkan* GetBufferResourceView(const SetResources& resources, const ezShaderResourceBinding& mapping);
  const ezGALTextureUnorderedAccessViewVulkan* GetTextureUAV(const SetResources& resources, const ezShaderResourceBinding& mapping);
  const ezGALBufferUnorderedAccessViewVulkan* GetBufferUAV(const SetResources& resources, const ezShaderResourceBinding& mapping);

private:
  ezGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device m_vkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;
  ezPipelineBarrierVulkan* m_pPipelineBarrier = nullptr;

  ezUniquePtr<ezUniformBufferPoolVulkan> m_pUniformBufferPool;

  // Cache flags.
  bool m_bPipelineStateDirty = true;
  bool m_bViewportDirty = true;
  bool m_bIndexBufferDirty = false;
  bool m_bDescriptorsDirty = false;
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bRenderPassActive = false; ///< #TODO_VULKAN Disabling and re-enabling the render pass is buggy as we might execute a clear twice.
  bool m_bClearSubmitted = false;   ///< Start render pass is lazy so if no draw call is executed we need to make sure the clear is executed anyways.
  bool m_bInsideCompute = false;    ///< Within BeginCompute / EndCompute block.
  bool m_bPushConstantsDirty = false;

  // Bound objects for deferred state flushes
  ezResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  ezResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  ezResourceCacheVulkan::ComputePipelineDesc m_ComputeDesc;
  vk::Framebuffer m_frameBuffer;
  vk::RenderPassBeginInfo m_renderPass;
  ezHybridArray<vk::ClearValue, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
  vk::ImageAspectFlags m_depthMask = {};
  ezUInt32 m_uiLayers = 0;

  vk::Viewport m_viewport;
  vk::Rect2D m_scissor;
  bool m_bScissorEnabled = false;

  const ezGALRenderTargetView* m_pBoundRenderTargets[EZ_GAL_MAX_RENDERTARGET_COUNT] = {};
  const ezGALRenderTargetView* m_pBoundDepthStencilTarget = nullptr;
  ezUInt32 m_uiBoundRenderTargetCount;

  const ezGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer m_pBoundVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  ezHybridArray<SetResources, 4> m_Resources;

  ezDeque<vk::DescriptorImageInfo> m_TextureAndSampler;
  ezHybridArray<vk::WriteDescriptorSet, 16> m_DescriptorWrites;
  ezHybridArray<vk::DescriptorSet, 4> m_DescriptorSets;

  ezDynamicArray<ezUInt8> m_PushConstants;

  ezDeque<vk::DescriptorBufferInfo> m_DynamicUniformBuffers;
  ezHybridArray<ezUInt32, 6> m_DynamicUniformBufferOffsets;
};
