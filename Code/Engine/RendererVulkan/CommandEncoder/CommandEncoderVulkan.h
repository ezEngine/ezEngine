
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class ezGALBlendStateVulkan;
class ezGALBufferVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALRasterizerStateVulkan;
class ezGALResourceViewVulkan;
class ezGALSamplerStateVulkan;
class ezGALShaderVulkan;
class ezGALUnorderedAccessViewVulkan;

/// \brief The Vulkan implementation of the command encoder.
template <typename Base>
class EZ_RENDERERVULKAN_DLL ezGALCommandEncoderVulkan : public Base
{
public:
protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALCommandEncoderVulkan(ezGALDevice& device);
  ~ezGALCommandEncoderVulkan();

  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) override;

  // Fence & Query functions

  virtual void InsertFencePlatform(const ezGALFence* pFence) override;
  virtual bool IsFenceReachedPlatform(const ezGALFence* pFence) override;
  virtual void WaitForFencePlatform(const ezGALFence* pFence) override;

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

  virtual void CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData) override;

  virtual void GenerateMipMapsPlatform(const ezGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  void FlushDeferredStateChanges();

  vk::Device m_vkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;

  const ezGALShaderVulkan* m_pCurrentShader;
  const ezGALBlendStateVulkan* m_pCurrentBlendState;
  const ezGALDepthStencilStateVulkan* m_pCurrentDepthStencilState;
  const ezGALRasterizerStateVulkan* m_pCurrentRasterizerState;
  const vk::PipelineVertexInputStateCreateInfo* m_pCurrentVertexLayout;
  vk::PrimitiveTopology m_currentPrimitiveTopology;

  bool m_bPipelineStateDirty = false;
  bool m_bFrameBufferDirty = false;
  bool m_bDescriptorsDirty = false;

  // Bound objects for deferred state flushes
  const ezGALRenderTargetView* m_pBoundRenderTargets[EZ_GAL_MAX_RENDERTARGET_COUNT];
  const ezGALRenderTargetView* m_pBoundDepthStencilTarget;
  ezUInt32 m_uiBoundRenderTargetCount;

  vk::Buffer m_pBoundVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;

  ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezUInt32 m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  const ezGALBufferVulkan* m_pBoundConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];
  ezGAL::ModifiedRange m_BoundConstantBuffersRange[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<const ezGALResourceViewVulkan*, 16> m_pBoundShaderResourceViews[ezGALShaderStage::ENUM_COUNT];
  ezGAL::ModifiedRange m_BoundShaderResourceViewsRange[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<const ezGALUnorderedAccessViewVulkan*, 16> m_pBoundUnoderedAccessViews;
  ezGAL::ModifiedRange m_pBoundUnoderedAccessViewsRange;

  const ezGALSamplerStateVulkan* m_pBoundSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SAMPLER_COUNT];
  ezGAL::ModifiedRange m_BoundSamplerStatesRange[ezGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/CommandEncoder/Implementation/CommandEncoderVulkan_inl.h>
