#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/DescriptorWritePoolVulkan.h>

#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererVulkan/Pools/UniformBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>

ezDescriptorWritePoolVulkan::ezDescriptorWritePoolVulkan(ezGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
}

void ezDescriptorWritePoolVulkan::WriteTransientDescriptor(vk::DescriptorSet descriptorSet, const ezGALBindGroupCreationDescription& desc, ezUniformBufferPoolVulkan* pUniformBufferPool)
{
  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(desc.m_hBindGroupLayout));
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = bindings.GetCount();

  EZ_LOCK(m_Mutex);
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = desc.m_BindGroupItems[i];
    vk::WriteDescriptorSet& write = WriteBindGroupItem(descriptorSet, binding, item, pUniformBufferPool);
    if (binding.m_ResourceType == ezGALShaderResourceType::ConstantBuffer)
    {
      // Offsets are moved into separate offset array so leave at 0. Dynamic uniform buffers need to be tagged in the layout, so we can't decide whether we use them or not depending on what buffer we have bound. Thus, everything is a dynamic uniform buffer and the offset must be provided via the bindDescriptorSets call for normal constant buffers too.
      // Offset does not need to be stored, as it is handled in ezGALCommandEncoderImplVulkan::FindDynamicUniformBuffers.
      const_cast<vk::DescriptorBufferInfo*>(write.pBufferInfo)->offset = 0;
    }
  }
}

void ezDescriptorWritePoolVulkan::WriteDescriptor(vk::DescriptorSet descriptorSet, const ezGALBindGroupCreationDescription& desc, ezDynamicArray<ezUInt32>& out_Offsets)
{
  out_Offsets.Clear();
  const ezGALBindGroupLayoutVulkan* pLayout = static_cast<const ezGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(desc.m_hBindGroupLayout));
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = bindings.GetCount();

  EZ_LOCK(m_Mutex);
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = desc.m_BindGroupItems[i];
    vk::WriteDescriptorSet& write = WriteBindGroupItem(descriptorSet, binding, item, nullptr);
    if (binding.m_ResourceType == ezGALShaderResourceType::ConstantBuffer)
    {
      out_Offsets.PushBack(write.pBufferInfo->offset);
      const_cast<vk::DescriptorBufferInfo*>(write.pBufferInfo)->offset = 0;
    }
  }
}

ezUInt32 ezDescriptorWritePoolVulkan::FlushWrites()
{
  EZ_LOCK(m_Mutex);
  ezUInt32 uiDescriptorWrites = 0;
  if (!m_DescriptorWrites.IsEmpty())
  {
    uiDescriptorWrites = m_DescriptorWrites.GetCount();
    m_pDevice->GetVulkanDevice().updateDescriptorSets(m_DescriptorWrites.GetCount(), m_DescriptorWrites.GetData(), 0, nullptr);
    m_DescriptorWrites.Clear();
    m_DescriptorImageInfos.Clear();
    m_DescriptorBufferInfos.Clear();
    m_DescriptorBufferViews.Clear();
  }
  return uiDescriptorWrites;
}

vk::WriteDescriptorSet& ezDescriptorWritePoolVulkan::WriteBindGroupItem(vk::DescriptorSet descriptorSet, const ezShaderResourceBinding& binding,
  const ezGALBindGroupItem& item, ezUniformBufferPoolVulkan* pUniformBufferPool)
{
  vk::WriteDescriptorSet& write = m_DescriptorWrites.ExpandAndGetRef();
  write.dstArrayElement = 0;
  write.descriptorType = ezConversionUtilsVulkan::GetDescriptorType(binding.m_ResourceType);
  write.dstBinding = binding.m_iSlot;
  write.dstSet = descriptorSet;
  write.descriptorCount = binding.m_uiArraySize;
  switch (binding.m_ResourceType)
  {
    case ezGALShaderResourceType::ConstantBuffer:
    {
      vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
      write.pBufferInfo = &bufferInfo;
      const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      if (pBuffer->GetDescription().m_BufferFlags.IsSet(ezGALBufferUsageFlags::Transient))
      {
        EZ_ASSERT_DEV(pUniformBufferPool != nullptr, "Bind group resources must not contain transient constant buffer references");
        bufferInfo = *pUniformBufferPool->GetBuffer(pBuffer);
      }
      else
      {
        bufferInfo = pBuffer->GetBufferInfo();
        bufferInfo.range = item.m_Buffer.m_BufferRange.m_uiByteCount;
      }
    }
    break;
    case ezGALShaderResourceType::Texture:
    case ezGALShaderResourceType::TextureRW:
    case ezGALShaderResourceType::TextureAndSampler:
    {
      vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
      write.pImageInfo = &imageInfo;
      const ezGALTextureVulkan* pTexture = static_cast<const ezGALTextureVulkan*>(m_pDevice->GetTexture(item.m_Texture.m_hTexture));
      imageInfo = pTexture->GetDescriptorImageInfo(item.m_Texture.m_TextureRange, binding.m_ResourceType, binding.m_TextureType, item.m_Texture.m_OverrideViewFormat);
      imageInfo.imageLayout = binding.m_ResourceType == ezGALShaderResourceType::TextureRW ? vk::ImageLayout::eGeneral : pTexture->GetPreferredLayout(ezConversionUtilsVulkan::GetDefaultLayout(pTexture->GetImageFormat()));

      if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
      {
        const ezGALSamplerStateVulkan* pSampler = static_cast<const ezGALSamplerStateVulkan*>(m_pDevice->GetSamplerState(item.m_Texture.m_hSampler));
        imageInfo.sampler = pSampler->GetImageInfo().sampler;
      }
    }
    break;
    case ezGALShaderResourceType::TexelBuffer:
    case ezGALShaderResourceType::TexelBufferRW:
    {
      vk::BufferView& bufferView = m_DescriptorBufferViews.ExpandAndGetRef();
      write.pTexelBufferView = &bufferView;
      const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      bufferView = pBuffer->GetTexelBufferView(item.m_Buffer.m_BufferRange, item.m_Buffer.m_OverrideTexelBufferFormat);
    }
    break;
    case ezGALShaderResourceType::StructuredBuffer:
    case ezGALShaderResourceType::ByteAddressBuffer:
    case ezGALShaderResourceType::StructuredBufferRW:
    case ezGALShaderResourceType::ByteAddressBufferRW:
    {
      vk::DescriptorBufferInfo& bufferInfo = m_DescriptorBufferInfos.ExpandAndGetRef();
      write.pBufferInfo = &bufferInfo;
      const ezGALBufferVulkan* pBuffer = static_cast<const ezGALBufferVulkan*>(m_pDevice->GetBuffer(item.m_Buffer.m_hBuffer));
      bufferInfo.buffer = pBuffer->GetBufferInfo().buffer;
      bufferInfo.offset = item.m_Buffer.m_BufferRange.m_uiByteOffset;
      bufferInfo.range = item.m_Buffer.m_BufferRange.m_uiByteCount;
    }
    break;
    case ezGALShaderResourceType::Sampler:
    {
      vk::DescriptorImageInfo& imageInfo = m_DescriptorImageInfos.ExpandAndGetRef();
      write.pImageInfo = &imageInfo;
      const ezGALSamplerStateVulkan* pSampler = static_cast<const ezGALSamplerStateVulkan*>(m_pDevice->GetSamplerState(item.m_Sampler.m_hSampler));
      imageInfo.sampler = pSampler->GetImageInfo().sampler;
    }
    break;
    default:
      break;
  }
  return write;
}
