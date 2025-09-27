#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Shader/BindGroup.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

#include <Foundation/Algorithm/HashStream.h>

ezUInt64 ezGALBindGroupCreationDescription::CalculateHash() const
{
  ezHashStreamWriter64 writer;
  writer << m_hBindGroupLayout.GetInternalID().m_Data;
  if (!m_BindGroupItems.IsEmpty())
  {
    auto data = m_BindGroupItems.GetByteArrayPtr();
    writer.WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
  }
  return writer.GetHashValue();
}
void ezGALBindGroupCreationDescription::AssertValidDescription(const ezGALDevice& m_Device) const
{
  const ezGALBindGroupLayout* pLayout = m_Device.GetBindGroupLayout(m_hBindGroupLayout);
  ezArrayPtr<const ezShaderResourceBinding> bindings = pLayout->GetDescription().m_ResourceBindings;
  ezArrayPtr<const ezGALBindGroupItem> items = m_BindGroupItems;
  EZ_ASSERT_ALWAYS(bindings.GetCount() == items.GetCount(), "Missmatch between bindings and item count");
  const ezUInt32 uiBindings = bindings.GetCount();
  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = bindings[i];
    const ezGALBindGroupItem& item = items[i];
    const ezBitflags<ezGALShaderResourceCategory> category = ezGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);

    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::Sampler:
      {
        EZ_ASSERT_ALWAYS(item.m_Flags.IsSet(ezGALBindGroupItemFlags::Sampler), "Item type does not match binding");
        const ezGALSamplerState* pSampler = m_Device.GetSamplerState(item.m_Sampler.m_hSampler);
        EZ_ASSERT_ALWAYS(pSampler != nullptr, "Invalid sampler state");
      }
      break;

      case ezGALShaderResourceType::ConstantBuffer:
      {
        EZ_ASSERT_ALWAYS(item.m_Flags.IsSet(ezGALBindGroupItemFlags::Buffer), "Item type does not match binding");
        const ezGALBuffer* pBuffer = m_Device.GetBuffer(item.m_Buffer.m_hBuffer);
        EZ_ASSERT_ALWAYS(pBuffer != nullptr, "Invalid buffer");
        EZ_ASSERT_ALWAYS(item.m_Buffer.m_OverrideTexelBufferFormat == ezGALResourceFormat::Invalid, "m_OverrideTexelBufferFormat must be Invalid for constant buffers");
        EZ_ASSERT_ALWAYS(item.m_Buffer.m_BufferRange.m_uiByteOffset == 0, "Byte offset for constant buffers not supported yet");
        EZ_ASSERT_ALWAYS(item.m_Buffer.m_BufferRange.m_uiByteCount == pBuffer->GetDescription().m_uiTotalSize, "Byte count for constant buffers not supported yet");
      }
      break;
      case ezGALShaderResourceType::TexelBuffer:
      case ezGALShaderResourceType::StructuredBuffer:
      case ezGALShaderResourceType::ByteAddressBuffer:
      case ezGALShaderResourceType::TexelBufferRW:
      case ezGALShaderResourceType::StructuredBufferRW:
      case ezGALShaderResourceType::ByteAddressBufferRW:
      {

        EZ_ASSERT_ALWAYS(item.m_Flags.IsSet(ezGALBindGroupItemFlags::Buffer), "Item type does not match binding");
        const ezGALBuffer* pBuffer = m_Device.GetBuffer(item.m_Buffer.m_hBuffer);
        EZ_ASSERT_ALWAYS(pBuffer != nullptr, "Invalid buffer");
        const ezGALBufferCreationDescription& bufferDesc = pBuffer->GetDescription();
        EZ_ASSERT_ALWAYS((binding.m_ResourceType == ezGALShaderResourceType::TexelBuffer || binding.m_ResourceType == ezGALShaderResourceType::TexelBufferRW) || item.m_Buffer.m_OverrideTexelBufferFormat == ezGALResourceFormat::Invalid, "m_OverrideTexelBufferFormat must be Invalid for non-texel buffers");
        EZ_ASSERT_ALWAYS((binding.m_ResourceType != ezGALShaderResourceType::TexelBuffer && binding.m_ResourceType != ezGALShaderResourceType::TexelBufferRW) || bufferDesc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::TexelBuffer), "TexelBuffer bindings are only supported on texel buffers");
        EZ_ASSERT_ALWAYS((binding.m_ResourceType != ezGALShaderResourceType::StructuredBuffer && binding.m_ResourceType != ezGALShaderResourceType::StructuredBufferRW) || bufferDesc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::StructuredBuffer), "StructuredBuffer bindings are only supported on structured buffers");
        EZ_ASSERT_ALWAYS((binding.m_ResourceType != ezGALShaderResourceType::ByteAddressBuffer && binding.m_ResourceType != ezGALShaderResourceType::ByteAddressBufferRW) || bufferDesc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ByteAddressBuffer), "ByteAddressBuffer bindings are only supported on byte address buffers");

        if (category.IsSet(ezGALShaderResourceCategory::BufferSRV))
        {
          EZ_ASSERT_ALWAYS(bufferDesc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource), "Buffer must have the ShaderResource flag set to be used as an SRV");
        }
        if (category.IsSet(ezGALShaderResourceCategory::BufferUAV))
        {
          EZ_ASSERT_ALWAYS(bufferDesc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess), "Buffer must have the UnorderedAccess flag set to be used as an UAV");
        }

        ezUInt32 uiBytesPerElement = 4; // ByteAddress must be multiple of 4
        if (binding.m_ResourceType == ezGALShaderResourceType::StructuredBuffer || binding.m_ResourceType == ezGALShaderResourceType::StructuredBufferRW)
        {
          uiBytesPerElement = bufferDesc.m_uiStructSize;
        }
        else if (binding.m_ResourceType == ezGALShaderResourceType::TexelBuffer || binding.m_ResourceType == ezGALShaderResourceType::TexelBufferRW)
        {
          const ezGALResourceFormat::Enum viewFormat = item.m_Buffer.m_OverrideTexelBufferFormat == ezGALResourceFormat::Invalid ? bufferDesc.m_Format : item.m_Buffer.m_OverrideTexelBufferFormat;
          uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(viewFormat) / 8;
        }

        ezGALBufferRange range = item.m_Buffer.m_BufferRange;
        if ((range.m_uiByteOffset % uiBytesPerElement) != 0)
        {
          EZ_REPORT_FAILURE("m_uiByteOffset {} is not a multiple of the element size {}", range.m_uiByteOffset, uiBytesPerElement);
        }
        if (range.m_uiByteOffset >= bufferDesc.m_uiTotalSize)
        {
          EZ_REPORT_FAILURE("m_uiByteOffset {} is too big for the buffer of size {}", range.m_uiByteOffset, bufferDesc.m_uiTotalSize);
        }

        if (range.m_uiByteCount != EZ_GAL_WHOLE_SIZE)
        {
          if ((range.m_uiByteCount % uiBytesPerElement) != 0)
          {
            EZ_REPORT_FAILURE("m_uiByteCount {} is not a multiple of the element size {}", range.m_uiByteCount, uiBytesPerElement);
          }
          if (range.m_uiByteOffset + range.m_uiByteCount > bufferDesc.m_uiTotalSize)
          {
            EZ_REPORT_FAILURE("m_uiByteOffset {} + m_uiByteCount {} = {} is too big for the buffer of size {}", range.m_uiByteOffset, range.m_uiByteCount, range.m_uiByteOffset + range.m_uiByteCount, bufferDesc.m_uiTotalSize);
          }
        }
      }
      break;
      case ezGALShaderResourceType::Texture:
      case ezGALShaderResourceType::TextureRW:
      case ezGALShaderResourceType::TextureAndSampler:
      {
        EZ_ASSERT_ALWAYS(item.m_Flags.IsSet(ezGALBindGroupItemFlags::Texture), "Item type does not match binding");
        const ezGALTexture* pTexture = m_Device.GetTexture(item.m_Texture.m_hTexture);
        EZ_ASSERT_ALWAYS(pTexture != nullptr, "Invalid texture");
        const auto& textureDesc = pTexture->GetDescription();

        if (item.m_Texture.m_OverrideViewFormat != ezGALResourceFormat::Invalid)
        {
          const ezEnum<ezGALResourceFormat> format = pTexture->GetDescription().m_Format;
          const ezEnum<ezGALResourceFormat> overrideFormat = item.m_Texture.m_OverrideViewFormat;
          EZ_ASSERT_ALWAYS(ezGALResourceFormat::GetBitsPerElement(format) == ezGALResourceFormat::GetBitsPerElement(overrideFormat), "Format override bits per element ({}) must match the same on the original format ({})", ezGALResourceFormat::GetBitsPerElement(overrideFormat), ezGALResourceFormat::GetBitsPerElement(format));
          EZ_ASSERT_ALWAYS(ezGALResourceFormat::GetChannelCount(format) == ezGALResourceFormat::GetChannelCount(overrideFormat), "Format override channel count ({}) must match the same on the original format ({})", ezGALResourceFormat::GetChannelCount(overrideFormat), ezGALResourceFormat::GetChannelCount(format));
        }
        // TODO item.m_Texture.m_OverrideTexelBufferFormat
        if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
        {
          const ezGALSamplerState* pSampler = m_Device.GetSamplerState(item.m_Texture.m_hSampler);
          EZ_ASSERT_ALWAYS(pSampler != nullptr, "Invalid sampler state");
        }

        const ezGALTextureRange range = item.m_Texture.m_TextureRange;

        if (!ezGALShaderTextureType::IsArray(binding.m_TextureType))
        {
          if (binding.m_TextureType == ezGALShaderTextureType::TextureCube)
          {
            EZ_ASSERT_ALWAYS(range.m_uiArraySlices == 6, "m_uiArraySlices must be 6 for a cube texture binding");
          }
          else
          {
            EZ_ASSERT_ALWAYS(range.m_uiArraySlices == 1, "m_uiArraySlices must be 1 for non array bindings");
          }
        }

        EZ_ASSERT_ALWAYS(ezGALShaderTextureType::IsMSAA(binding.m_TextureType) == (textureDesc.m_SampleCount != ezGALMSAASampleCount::None), "MSAA missmatch between texture and binding");

        if (category.IsSet(ezGALShaderResourceCategory::TextureSRV))
        {
          EZ_ASSERT_ALWAYS(textureDesc.m_bAllowShaderResourceView, "Texture must have the m_bAllowShaderResourceView bool set to be used as an SRV");
        }
        if (category.IsSet(ezGALShaderResourceCategory::TextureUAV))
        {
          EZ_ASSERT_ALWAYS(textureDesc.m_bAllowUAV, "Texture must have the m_bAllowUAV bool set to be used as an UAV");
        }
        const ezUInt32 uiSlices = (textureDesc.m_Type == ezGALTextureType::TextureCube || textureDesc.m_Type == ezGALTextureType::TextureCubeArray) ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
        EZ_ASSERT_ALWAYS(textureDesc.m_Type != ezGALTextureType::Texture2DProxy, "Proxy textures must be resolved to their base texture before binding");
        EZ_ASSERT_ALWAYS(range.m_uiBaseArraySlice < uiSlices, "Base array slice is out of bounds");
        EZ_ASSERT_ALWAYS(range.m_uiBaseMipLevel < textureDesc.m_uiMipLevelCount, "Base array slice is out of bounds");
        EZ_ASSERT_ALWAYS(range.m_uiMipLevels > 0, "Mip level count must be greater than 0");
        EZ_ASSERT_ALWAYS(range.m_uiArraySlices > 0, "Array slices must be greater than 0");
        EZ_ASSERT_ALWAYS(range.m_uiMipLevels == EZ_GAL_ALL_MIP_LEVELS || static_cast<ezUInt32>(range.m_uiBaseMipLevel) + range.m_uiMipLevels <= textureDesc.m_uiMipLevelCount, "Mip level range is out of bounds");

        EZ_ASSERT_ALWAYS(range.m_uiArraySlices == EZ_GAL_ALL_ARRAY_SLICES || static_cast<ezUInt32>(range.m_uiBaseArraySlice) + range.m_uiArraySlices <= uiSlices, "Array slice range is out of bounds");
        EZ_ASSERT_ALWAYS(binding.m_TextureType != ezGALShaderTextureType::TextureCube || range.m_uiArraySlices == 6, "Cube textures must have 6 as array slices");
        EZ_ASSERT_ALWAYS(binding.m_TextureType != ezGALShaderTextureType::TextureCubeArray || (range.m_uiArraySlices % 6) == 0, "Cube array textures must have array slices that are multiple of 6");
      }
      break;
      case ezGALShaderResourceType::Unknown:
      case ezGALShaderResourceType::PushConstants:
      default:
        EZ_REPORT_FAILURE("Unsupported Shader Resource Type: {}", binding.m_ResourceType.GetValue());
        break;
    }
  }
}
