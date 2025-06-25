#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

namespace
{
  vk::ImageSubresourceRange CreateSubRange(const vk::ImageSubresourceRange& fullRange, ezUInt32 uiLayer, ezUInt32 uiMipLevel)
  {
    vk::ImageSubresourceRange subRange = fullRange;
    subRange.baseArrayLayer = uiLayer;
    subRange.baseMipLevel = uiMipLevel;
    subRange.layerCount = 1;
    subRange.levelCount = 1;
    return subRange;
  }
} // namespace

ezPipelineBarrierVulkan::ezPipelineBarrierVulkan(ezAllocator* pAllocator)
  : m_bufferBarriers(pAllocator)
  , m_imageBarriers(pAllocator)
  , m_imageState(pAllocator)
  , m_bufferState(pAllocator)
{
}

void ezPipelineBarrierVulkan::SetCommandBuffer(vk::CommandBuffer* pCommandBuffer)
{
  m_pCommandBuffer = pCommandBuffer;
}

void ezPipelineBarrierVulkan::Flush()
{
  EZ_PROFILE_SCOPE("Flush");

  if (m_srcStageMask || m_dstStageMask)
  {
    if (!m_srcStageMask)
      m_srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
    if (!m_dstStageMask)
      m_dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

    vk::MemoryBarrier memoryBarrier;
    memoryBarrier.srcAccessMask = m_srcAccess;
    memoryBarrier.dstAccessMask = m_dstAccess;
    bool bHasMemoryBarrier = (m_srcAccess | m_dstAccess) != vk::AccessFlagBits::eNone;

    m_pCommandBuffer->pipelineBarrier(m_srcStageMask, m_dstStageMask, vk::DependencyFlags(), bHasMemoryBarrier ? 1 : 0, bHasMemoryBarrier ? &memoryBarrier : nullptr, m_bufferBarriers.GetCount(), m_bufferBarriers.IsEmpty() ? nullptr : m_bufferBarriers.GetData(), m_imageBarriers.GetCount(), m_imageBarriers.IsEmpty() ? nullptr : m_imageBarriers.GetData());

#ifdef VK_LOG_LAYOUT_CHANGES
    ezLog::Warning("Flush: [m{}] {}|{} -> {}|{}", bHasMemoryBarrier, vk::to_string(m_srcStageMask).c_str(), vk::to_string(m_srcAccess).c_str(), vk::to_string(m_dstStageMask).c_str(), vk::to_string(m_dstAccess).c_str());

    for (vk::ImageMemoryBarrier& img : m_imageBarriers)
    {
      ezLog::Warning("Layout Changed: {} [{},{}]: {} [{}] -> {} [{}]", ezArgP(img.image), img.subresourceRange.baseMipLevel, img.subresourceRange.levelCount, vk::to_string(img.oldLayout).c_str(), vk::to_string(img.srcAccessMask).c_str(), vk::to_string(img.newLayout).c_str(), vk::to_string(img.dstAccessMask).c_str());
    }
#endif

    m_srcAccess = {};
    m_dstAccess = {};
    m_bufferState.Clear();

    m_srcStageMask = {};
    m_dstStageMask = {};
    m_bufferBarriers.Clear();
    m_imageBarriers.Clear();

    for (auto it = m_imageState.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_dirty.ClearAllBits();
    }
    FullBarrier();
  }
}

void ezPipelineBarrierVulkan::Submit()
{
  Flush();
  // #TODO_VULKAN reset everything to default except for present.
  for (auto it = m_imageState.GetIterator(); it.IsValid();)
  {
    ImageState& imageState = it.Value();
    const vk::ImageLayout commonLayout = imageState.m_subElementLayout[0].m_layout;
    bool bDifferent = false;

    const ezUInt32 uiCount = it.Value().m_subElementLayout.GetCount();
    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      // Submit is always a full memory barrier so we can clear the current access mask and stages.
      SubElementState& state = imageState.m_subElementLayout[i];
      state.m_accessMask = {};
      state.m_stages = vk::PipelineStageFlagBits::eTopOfPipe;
      bDifferent |= (state.m_layout != commonLayout);
    }

    auto itOld = it;
    ++it;
    if (!bDifferent)
    {
      if (uiCount > 1)
      {
        // Collapse to full layout. Not need to clear as all values are the same
        imageState.m_dirty.SetCount(1);
        imageState.m_subElementLayout.SetCount(1);
      }

      if (imageState.m_pTexture->GetPreferredLayout() == commonLayout)
      {
        // Common layout matches the preferred layout of the texture, no need to continue tracking it.
        m_imageState.Remove(itOld);
      }
    }
  }
  m_pCommandBuffer = nullptr;
}

bool ezPipelineBarrierVulkan::IsDirty(vk::Image image, const vk::ImageSubresourceRange& subResources) const
{
  if (const ImageState* pState = m_imageState.GetValue(image))
  {
    return IsDirtyInternal(*pState, subResources);
  }
  return false;
}

bool ezPipelineBarrierVulkan::IsDirty() const
{
  return m_srcStageMask || m_dstStageMask;
}

bool ezPipelineBarrierVulkan::IsDirty(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize length, vk::AccessFlags dstAccess)
{
  BufferState* pState = m_bufferState.GetValue(buffer);
  if (pState)
  {
    SubBufferState subState;
    subState.m_offset = offset;
    subState.m_length = length;
    subState.m_stages = {};
    subState.m_accessMask = dstAccess;
    return IsDirtyInternal(*pState, subState);
  }
  return false;
}

void ezPipelineBarrierVulkan::AccessBuffer(const ezGALBufferVulkan* pBuffer, vk::DeviceSize offset, vk::DeviceSize length, vk::PipelineStageFlags srcStages, vk::AccessFlags srcAccess, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess)
{
  SubBufferState subState;
  subState.m_offset = offset;
  subState.m_length = length;
  subState.m_stages = dstStages;
  subState.m_accessMask = dstAccess;

  BufferState* pState = m_bufferState.GetValue(pBuffer->GetVkBuffer());
  if (pState)
  {
    if (IsDirtyInternal(*pState, subState))
    {
      pState = nullptr;
      Flush();
    }
  }

  if (!pState)
  {
    BufferState state;
    state.m_pBuffer = pBuffer;
    pState = &m_bufferState.Insert(pBuffer->GetVkBuffer(), state).Value();
  }

  AddBufferBarrierInternal(pBuffer->GetVkBuffer(), offset, length, srcStages, srcAccess, dstStages, dstAccess);
  pState->m_subBufferState.PushBack(subState);
  pState->m_dirty.SetCount(pState->m_subBufferState.GetCount(), true);
}

void ezPipelineBarrierVulkan::EnsureImageLayout(const ezGALTextureVulkan* pTexture, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource)
{
  EnsureImageLayout(pTexture, pTexture->GetFullRange(), dstLayout, dstStages, dstAccess, bDiscardSource);
}

void ezPipelineBarrierVulkan::EnsureImageLayout(const ezGALRenderTargetViewVulkan* pTextureView, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource)
{
  auto pTexture = static_cast<const ezGALTextureVulkan*>(pTextureView->GetTexture()->GetParentResource());
  EnsureImageLayout(pTexture, pTextureView->GetRange(), dstLayout, dstStages, dstAccess, bDiscardSource);
}

void ezPipelineBarrierVulkan::EnsureImageLayout(const ezGALTextureVulkan* pTexture, ezGALTextureRange range, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource)
{
  vk::ImageSubresourceRange vkRange = ezConversionUtilsVulkan::GetSubresourceRange(pTexture->GetDescription().m_Format, range);
  auto pTexture2 = static_cast<const ezGALTextureVulkan*>(pTexture->GetParentResource());
  EnsureImageLayout(pTexture2, vkRange, dstLayout, dstStages, dstAccess, bDiscardSource);
}

void ezPipelineBarrierVulkan::EnsureImageLayout(const ezGALTextureVulkan* pTexture, vk::ImageSubresourceRange subResources, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess, bool bDiscardSource)
{
  const vk::ImageSubresourceRange fullRange = pTexture->GetFullRange();
  if (ezGALResourceFormat::IsStencilFormat(pTexture->GetDescription().m_Format))
  {
    // Vulkan spec forces us to always transition depth and stencil at the same time.
    subResources.aspectMask |= vk::ImageAspectFlagBits::eStencil | vk::ImageAspectFlagBits::eDepth;
  }

  const bool bIsFullRange = subResources == fullRange;

  ImageState* pState = m_imageState.GetValue(pTexture->GetImage());
  if (pState)
  {
    if (IsDirtyInternal(*pState, subResources))
    {
      // We can't switch the layout twice in one barrier so we need to flush any overlapping changes first.
      Flush();
    }

    if (bIsFullRange)
    {
      if (pState->m_subElementLayout.GetCount() == 1)
      {
        // full range to full range
        SubElementState& subState = pState->m_subElementLayout[0];
        if (AddImageBarrierInternal(pTexture->GetImage(), subResources, subState.m_layout, subState.m_accessMask, dstLayout, dstAccess, bDiscardSource))
        {
          m_srcStageMask |= subState.m_stages;
          m_dstStageMask |= dstStages;

          subState.m_stages = dstStages;
          subState.m_accessMask = dstAccess;
          subState.m_layout = dstLayout;
          pState->m_dirty.SetBit(0);
        }
      }
      else
      {
        // partial to full range
        bool bDirty = false;
        vk::PipelineStageFlags srcStages;
        const ezUInt32 uiLastLayer = subResources.baseArrayLayer + subResources.layerCount;
        const ezUInt32 uiLastLevel = subResources.baseMipLevel + subResources.levelCount;
        for (ezUInt32 uiLayer = subResources.baseArrayLayer; uiLayer < uiLastLayer; uiLayer++)
        {
          for (ezUInt32 uiMipLevel = subResources.baseMipLevel; uiMipLevel < uiLastLevel; uiMipLevel++)
          {
            const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * fullRange.levelCount;
            SubElementState& subState = pState->m_subElementLayout[uiSubresourceIndex];
            srcStages |= subState.m_stages;

            bDirty |= AddImageBarrierInternal(pTexture->GetImage(), CreateSubRange(fullRange, uiLayer, uiMipLevel), subState.m_layout, subState.m_accessMask, dstLayout, dstAccess, bDiscardSource);
          }
        }
        if (bDirty)
        {
          m_dstStageMask |= dstStages;
          m_srcStageMask |= srcStages;
        }
        pState->m_dirty.Clear();
        pState->m_dirty.SetCount(1, bDirty);
        pState->m_subElementLayout.Clear();
        pState->m_subElementLayout.SetCount(1, {dstStages, dstAccess, dstLayout});
      }
    }
    else
    {
      if (pState->m_subElementLayout.GetCount() == 1)
      {
        // full to partial range
        const SubElementState subState = pState->m_subElementLayout[0];

        const bool bDirty = AddImageBarrierInternal(pTexture->GetImage(), subResources, subState.m_layout, subState.m_accessMask, dstLayout, dstAccess, bDiscardSource);

        if (bDirty)
        {
          m_srcStageMask |= subState.m_stages;
          m_dstStageMask |= dstStages;

          pState->m_dirty.Clear();
          pState->m_dirty.SetCount(fullRange.levelCount * fullRange.layerCount, false);
          pState->m_subElementLayout.Clear();
          pState->m_subElementLayout.SetCount(fullRange.levelCount * fullRange.layerCount, subState);

          const ezUInt32 uiLastLayer = subResources.baseArrayLayer + subResources.layerCount;
          const ezUInt32 uiLastLevel = subResources.baseMipLevel + subResources.levelCount;
          for (ezUInt32 uiLayer = subResources.baseArrayLayer; uiLayer < uiLastLayer; uiLayer++)
          {
            for (ezUInt32 uiMipLevel = subResources.baseMipLevel; uiMipLevel < uiLastLevel; uiMipLevel++)
            {
              const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * fullRange.levelCount;
              pState->m_dirty.SetBit(uiSubresourceIndex);
              pState->m_subElementLayout[uiSubresourceIndex] = {dstStages, dstAccess, dstLayout};
            }
          }
        }
      }
      else
      {
        // partial to partial range
        bool bDirty = false;
        vk::PipelineStageFlags srcStages;

        const ezUInt32 uiLastLayer = subResources.baseArrayLayer + subResources.layerCount;
        const ezUInt32 uiLastLevel = subResources.baseMipLevel + subResources.levelCount;
        for (ezUInt32 uiLayer = subResources.baseArrayLayer; uiLayer < uiLastLayer; uiLayer++)
        {
          for (ezUInt32 uiMipLevel = subResources.baseMipLevel; uiMipLevel < uiLastLevel; uiMipLevel++)
          {
            const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * fullRange.levelCount;

            SubElementState& subState = pState->m_subElementLayout[uiSubresourceIndex];
            srcStages |= subState.m_stages;

            if (AddImageBarrierInternal(pTexture->GetImage(), CreateSubRange(fullRange, uiLayer, uiMipLevel), subState.m_layout, subState.m_accessMask, dstLayout, dstAccess, bDiscardSource))
            {
              bDirty = true;
              pState->m_dirty.SetBit(uiSubresourceIndex);
              subState = {dstStages, dstAccess, dstLayout};
            }
          }
        }

        if (bDirty)
        {
          m_srcStageMask |= srcStages;
          m_dstStageMask |= dstStages;
        }
      }
    }
  }
  else
  {
    const vk::ImageLayout srcLayout = pTexture->GetPreferredLayout();
    if (srcLayout == dstLayout && !(s_writeAccess & dstAccess))
    {
      // We can early out of read-only access transitions into the current layout.
      return;
    }

    m_srcStageMask |= pTexture->GetUsedByPipelineStage();
    m_dstStageMask |= dstStages;

    ImageState state;
    state.m_pTexture = pTexture;
    if (bIsFullRange)
    {
      state.m_dirty.SetCount(1, true);
      state.m_subElementLayout.SetCount(1, {dstStages, dstAccess, dstLayout});

      if (srcLayout != dstLayout || (s_writeAccess & dstAccess))
      {
        AddImageBarrierInternal(pTexture->GetImage(), subResources, srcLayout, pTexture->GetAccessMask(), dstLayout, dstAccess, bDiscardSource);
      }
    }
    else
    {
      state.m_dirty.SetCount(fullRange.levelCount * fullRange.layerCount, false);
      state.m_subElementLayout.SetCount(fullRange.levelCount * fullRange.layerCount, {vk::PipelineStageFlagBits::eTopOfPipe, {}, srcLayout});

      const ezUInt32 uiLastLayer = subResources.baseArrayLayer + subResources.layerCount;
      const ezUInt32 uiLastLevel = subResources.baseMipLevel + subResources.levelCount;
      for (ezUInt32 uiLayer = subResources.baseArrayLayer; uiLayer < uiLastLayer; uiLayer++)
      {
        for (ezUInt32 uiMipLevel = subResources.baseMipLevel; uiMipLevel < uiLastLevel; uiMipLevel++)
        {
          const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * fullRange.levelCount;
          state.m_dirty.SetBit(uiSubresourceIndex);
          state.m_subElementLayout[uiSubresourceIndex] = {dstStages, dstAccess, dstLayout};
        }
      }
      if (srcLayout != dstLayout || (s_writeAccess & dstAccess))
      {
        AddImageBarrierInternal(pTexture->GetImage(), subResources, srcLayout, pTexture->GetAccessMask(), dstLayout, dstAccess, bDiscardSource);
      }
    }
    m_imageState.Insert(pTexture->GetImage(), state);
  }
}

void ezPipelineBarrierVulkan::SetInitialImageState(const ezGALTextureVulkan* pTexture, vk::ImageLayout dstLayout, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess)
{
  auto it = m_imageState.Find(pTexture->GetImage());

  // A Vulkan runtime is free to provide us with the very same native object if we request a resize but didn't actually change the size, in which case we ignore that we are already tacking this resource.
  EZ_ASSERT_DEBUG(!it.IsValid() || it.Value().m_pTexture->GetDescription().m_pExisitingNativeObject != nullptr, "Can't set initial state, texture is already tracked.");

  ImageState state;
  state.m_pTexture = pTexture;
  state.m_dirty.SetCount(1, false);
  state.m_subElementLayout.SetCount(1, {dstStages, dstAccess, dstLayout});

  m_imageState.Insert(pTexture->GetImage(), state);
}

void ezPipelineBarrierVulkan::TextureDestroyed(const ezGALTextureVulkan* pTexture)
{
  auto it = m_imageState.Find(pTexture->GetImage());
  if (it.IsValid())
  {
    // If the resource is dirty we don't care about any dirty state, just remove the barriers again from the list.
    if (it.Value().m_dirty.IsAnyBitSet())
    {
      vk::Image img = pTexture->GetImage();
      for (ezInt32 i = ((ezInt32)m_imageBarriers.GetCount()) - 1; i >= 0; --i)
      {
        if (m_imageBarriers[i].image == img)
        {
          m_imageBarriers.RemoveAtAndSwap(i);
        }
      }
    }

    m_imageState.Remove(it);
  }
}

void ezPipelineBarrierVulkan::BufferDestroyed(const ezGALBufferVulkan* pBuffer)
{
  auto it = m_bufferState.Find(pBuffer->GetVkBuffer());
  if (it.IsValid())
  {
    // #TODO_VULKAN do we need to flush here in case the resource is dirty?
    m_bufferState.Remove(it);
  }
}

void ezPipelineBarrierVulkan::FullBarrier()
{
  vk::MemoryBarrier memoryBarrier;
  memoryBarrier.srcAccessMask = s_writeAccess | s_readAccess;
  memoryBarrier.dstAccessMask = s_writeAccess | s_readAccess;

  m_pCommandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

bool ezPipelineBarrierVulkan::AddBufferBarrierInternal(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize length, vk::PipelineStageFlags srcStages, vk::AccessFlags srcAccess, vk::PipelineStageFlags dstStages, vk::AccessFlags dstAccess)
{
  // According to https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/, no GPU supports VkBufferMemoryBarrier so it's best to just use a memory barrier. This is corroborated by https://github.com/doitsujin/dxvk/blob/master/src/dxvk/dxvk_barrier.cpp which also omits using VkBufferMemoryBarrier.

  if ((((srcAccess | dstAccess) & s_writeAccess)) == vk::AccessFlagBits::eNone)
    return false;

  m_srcStageMask |= srcStages;
  m_dstStageMask |= dstStages;

  m_srcAccess |= srcAccess;
  m_dstAccess |= dstAccess;
  return true;
}

bool ezPipelineBarrierVulkan::IsDirtyInternal(const BufferState& state, const SubBufferState& subState) const
{
  const ezUInt32 uiCount = state.m_subBufferState.GetCount();
  for (ezUInt32 i = 0; i < uiCount; i++)
  {
    const SubBufferState& current = state.m_subBufferState[i];

    // overlap
    if (subState.m_offset + subState.m_length > current.m_offset && current.m_offset + current.m_length > subState.m_offset)
    {
      if (state.m_dirty.IsBitSet(i) && (subState.m_accessMask & s_writeAccess | current.m_accessMask & s_writeAccess))
        return true;
    }
  }
  return false;
}

bool ezPipelineBarrierVulkan::AddImageBarrierInternal(vk::Image image, const vk::ImageSubresourceRange& subResources, vk::ImageLayout srcLayout, vk::AccessFlags srcAccess, vk::ImageLayout dstLayout, vk::AccessFlags dstAccess, bool bDiscardSource)
{
  if (srcLayout == dstLayout && srcAccess == dstAccess && !(s_writeAccess & srcAccess))
    return false;

  vk::ImageMemoryBarrier& imageBarrier = m_imageBarriers.ExpandAndGetRef();

  imageBarrier.srcAccessMask = srcAccess;
  imageBarrier.dstAccessMask = dstAccess;
  imageBarrier.oldLayout = bDiscardSource ? vk::ImageLayout::eUndefined : srcLayout;
  imageBarrier.newLayout = dstLayout;
  imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.image = image;
  imageBarrier.subresourceRange = subResources;
  return true;
}

bool ezPipelineBarrierVulkan::IsDirtyInternal(const ImageState& state, const vk::ImageSubresourceRange& subResources) const
{
  if (state.m_dirty.GetCount() == 1)
  {
    return state.m_dirty.IsBitSet(0);
  }
  else
  {
    ezUInt32 uiLevelCount = state.m_pTexture->GetDescription().m_uiMipLevelCount;

    const ezUInt32 uiLastLayer = subResources.baseArrayLayer + subResources.layerCount;
    const ezUInt32 uiLastLevel = subResources.baseMipLevel + subResources.levelCount;
    for (ezUInt32 uiLayer = subResources.baseArrayLayer; uiLayer < uiLastLayer; uiLayer++)
    {
      for (ezUInt32 uiMipLevel = subResources.baseMipLevel; uiMipLevel < uiLastLevel; uiMipLevel++)
      {
        const ezUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * uiLevelCount;
        if (state.m_dirty.IsBitSet(uiSubresourceIndex))
        {
          return true;
        }
      }
    }
    return false;
  }
}
