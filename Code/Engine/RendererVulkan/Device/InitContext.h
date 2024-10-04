
#pragma once

#include <Foundation/Types/UniquePtr.h>

class ezGALDeviceVulkan;
class ezPipelineBarrierVulkan;
class ezCommandBufferPoolVulkan;
class ezStagingBufferPoolVulkan;

/// \brief Thread-safe context for initializing resources. Records a command buffer that transitions all newly created resources into their initial state.
class ezInitContextVulkan
{
public:
  ezInitContextVulkan(ezGALDeviceVulkan* pDevice);
  ~ezInitContextVulkan();

  /// \brief Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// \brief Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const ezGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData);

  void UpdateTexture(const ezGALTextureVulkan* pTexture, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData);

  /// \brief Needs to be called by the ezGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const ezGALTextureVulkan* pTexture);


  void InitBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> pInitialData);

  void UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData);

  /// \brief Used by ezUniformBufferPoolVulkan to write the entire uniform scratch pool to the GPU
  /// \param gpuBuffer The device local buffer to update.
  /// \param stagingBuffer The staging buffer that contains the data to be copied to gpuBuffer. If null, buffer is CPU writable and already contains the data.
  /// \param uiDataSize The size of the data to be copied from stagingBuffer to gpuBuffer.
  void UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, ezUInt32 uiDataSize);

private:
  void EnsureCommandBufferExists();

  ezGALDeviceVulkan* m_pDevice = nullptr;

  ezMutex m_Lock;
  ezDynamicArray<ezUInt8> m_TempData;
  vk::CommandBuffer m_currentCommandBuffer;
  ezUniquePtr<ezPipelineBarrierVulkan> m_pPipelineBarrier;
  ezUniquePtr<ezCommandBufferPoolVulkan> m_pCommandBufferPool;
  ezUniquePtr<ezStagingBufferPoolVulkan> m_pStagingBufferPool;
};
