
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

  void AfterBeginFrame();

  ezMutex& AccessLock() { return m_Lock; }

  /// \brief Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// \brief Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const ezGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData);

  /// \brief Needs to be called by the ezGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const ezGALTextureVulkan* pTexture);

  /// \brief Initializes a buffer with the given data.
  /// \param pBuffer The buffer to initialize.
  /// \param pInitialData The initial data that the buffer should be filled with.
  void InitBuffer(const ezGALBufferVulkan* pBuffer, ezConstByteArrayPtr pInitialData);

  /// \brief Updates a texture region
  void UpdateTexture(const ezGALTextureVulkan* pTexture, const ezGALTextureSubresource& subresource, const ezBoundingBoxu32& box, const ezGALSystemMemoryDescription& sourceData);

  /// \brief Updates a buffer range
  /// \param pBuffer The buffer to update.
  /// \param uiOffset The offset inside the buffer where the new data should be placed.
  /// \param pSourceData The new data to update the buffer with.
  void UpdateBuffer(const ezGALBufferVulkan* pBuffer, ezUInt32 uiOffset, ezConstByteArrayPtr pSourceData);

  /// \brief Used by ezUniformBufferPoolVulkan to write the entire uniform scratch pool to the GPU
  /// \param gpuBuffer The device local buffer to update.
  /// \param stagingBuffer The staging buffer that contains the data to be copied to gpuBuffer. If null, buffer is CPU writable and already contains the data.
  /// \param uiOffset Offset in the buffer.
  /// \param uiSize The size of the data to be copied from stagingBuffer to gpuBuffer.
  void UpdateDynamicUniformBuffer(vk::Buffer gpuBuffer, vk::Buffer stagingBuffer, ezUInt32 uiOffset, ezUInt32 uiSize);

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
