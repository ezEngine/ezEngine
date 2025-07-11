#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <vulkan/vulkan.hpp>

class ezGALBlendStateVulkan;
class ezGALBufferVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALRasterizerStateVulkan;
class ezGALBufferResourceViewVulkan;
class ezGALSamplerStateVulkan;
class ezGALShaderVulkan;
class ezGALDeviceVulkan;
class ezFenceQueueVulkan;
class ezGALGraphicsPipelineVulkan;
class ezGALComputePipelineVulkan;
struct ezGALBindGroupCreationDescription;

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
  virtual void SetBindGroupPlatform(ezUInt32 uiBindGroup, const ezGALBindGroupCreationDescription& bindGroup) override;
  virtual void SetPushConstantsPlatform(ezArrayPtr<const ezUInt8> data) override;

  // GPU -> CPU query functions

  virtual ezGALTimestampHandle InsertTimestampPlatform() override;
  virtual ezGALOcclusionHandle BeginOcclusionQueryPlatform(ezEnum<ezGALQueryType> type) override;
  virtual void EndOcclusionQueryPlatform(ezGALOcclusionHandle hOcclusion) override;
  virtual ezGALFenceHandle InsertFencePlatform() override;


  // Resource update functions

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const ezGALReadbackTexture* pDestination, const ezGALTexture* pSource) override;
  virtual void ReadbackBufferPlatform(const ezGALReadbackBuffer* pDestination, const ezGALBuffer* pSource) override;

  virtual void GenerateMipMapsPlatform(const ezGALTexture* pTexture, ezGALTextureRange range) override;

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

  virtual void SetGraphicsPipelinePlatform(const ezGALGraphicsPipeline* pGraphicsPipeline) override;
  virtual void SetComputePipelinePlatform(const ezGALComputePipeline* pComputePipeline) override;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) override;
  virtual void SetStencilReferencePlatform(ezUInt8 uiStencilRefValue) override;

  struct Statistics
  {
    ezUInt32 m_uiDescriptorSetsCreated = 0;
    ezUInt32 m_uiDescriptorSetsUpdated = 0;
    ezUInt32 m_uiDescriptorSetsReused = 0;
    ezUInt32 m_uiDescriptorWrites = 0;
    ezUInt32 m_uiDynamicUniformBufferChanged = 0;
  };
  Statistics GetAndResetStatistics();

private:
  /// \brief To be able to cache descriptor sets, we not only need the bind group description but also the currently used dynamic uniform buffers used by transient constant buffers.
  /// All constant buffers in EZ are marked as dynamic in the layout so we need to provide offsets for each slot, no mater if a normal or transient constant buffer is bound to a slot.
  struct DynamicOffsets
  {
    ezHybridArray<const ezGALBufferVulkan*, 6> m_DynamicUniformBuffers; ///< Constant buffers in order of appearance in the bind group. Normal constant buffers write a nullptr here as they have fixed offsets and will never have to be updated. Only updated once via FindDynamicUniformBuffers.
    ezHybridArray<vk::Buffer, 6> m_DynamicUniformVkBuffers;             ///< Current vk::Buffer for each dynamic uniform buffer in m_DynamicUniformBuffers. Updated via UpdateDynamicUniformBufferOffsets. If any of these change, a new descriptor has to be created.
    ezHybridArray<ezUInt32, 6> m_DynamicUniformBufferOffsets;           ///< Offsets in this bind group. Normal constant buffers have fixed offsets determined in FindDynamicUniformBuffers which never change. Transient constant buffer offsets are updated with each UpdateDynamicUniformBufferOffsets call.
  };

  ezResult FlushDeferredStateChanges();
  void FindDynamicUniformBuffers(const ezGALBindGroupCreationDescription& desc, DynamicOffsets& out_offsets);
  static ezUInt64 HashBindGroup(const ezGALBindGroupCreationDescription& desc, const DynamicOffsets& offsets);
  vk::DescriptorSet CreateDescriptorSet(const ezGALBindGroupCreationDescription& desc, const DynamicOffsets& offsets);
  void EnsureBindGroupTextureLayout(const ezGALBindGroupCreationDescription& desc);

  enum class DynamicUniformBufferChanges
  {
    None,           ///< Neither offsets nor buffers have changed.
    OffsetsChanged, ///< Offsets have changed, call bindDescriptorSets with new offsets.
    BuffersChanged, ///< Buffers have changed, create new descriptor set for new buffers. This should only happen if we exhaust the current dynamic uniform buffer and request a new one from the pool.
  };
  DynamicUniformBufferChanges UpdateDynamicUniformBufferOffsets(DynamicOffsets& ref_offsets);

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
  bool m_BindGroupDirty[EZ_GAL_MAX_BIND_GROUPS] = {};
  bool m_bDynamicOffsetsDirty = false;
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bRenderPassActive = false; ///< #TODO_VULKAN Disabling and re-enabling the render pass is buggy as we might execute a clear twice.
  bool m_bClearSubmitted = false;   ///< Start render pass is lazy so if no draw call is executed we need to make sure the clear is executed anyways.
  bool m_bInsideCompute = false;    ///< Within BeginCompute / EndCompute block.
  bool m_bPushConstantsDirty = false;

  // Bound objects for deferred state flushes
  const ezGALShaderVulkan* m_pShader = nullptr;
  const ezGALGraphicsPipelineVulkan* m_pGraphicsPipeline = nullptr;
  const ezGALComputePipelineVulkan* m_pComputePipeline = nullptr;

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

  // Bind Groups
  ezGALBindGroupCreationDescription m_BindGroups[EZ_GAL_MAX_BIND_GROUPS];
  DynamicOffsets m_DynamicOffsets[EZ_GAL_MAX_BIND_GROUPS];

  // Descriptor Writes
  ezDynamicArray<vk::WriteDescriptorSet> m_DescriptorWrites;
  ezDeque<vk::DescriptorImageInfo> m_DescriptorImageInfos;
  ezDeque<vk::DescriptorBufferInfo> m_DescriptorBufferInfos;
  ezDeque<vk::BufferView> m_DescriptorBufferViews;

  // Actual bound descriptor sets
  ezHashTable<ezUInt64, vk::DescriptorSet> m_DescriptorCache;
  vk::DescriptorSet m_DescriptorSets[EZ_GAL_MAX_BIND_GROUPS];

  ezDynamicArray<ezUInt8> m_PushConstants;

  Statistics m_Statistics;
};
