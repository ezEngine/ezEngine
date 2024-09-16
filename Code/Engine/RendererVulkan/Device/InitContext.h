
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

  void InitBuffer(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> pInitialData);

  /// \brief Needs to be called by the ezGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const ezGALTextureVulkan* pTexture);

  void UpdateDynamicUniformBuffer(vk::Buffer buffer, vk::Buffer stagingBuffer, ezUInt32 uiDataSize);

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
