#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

#include "RendererFoundation/Device/Device.h"

#include <RendererFoundation/Resources/ResourceFormats.h>

ezResult ezGALTextureCreationDescription::Validate(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> initialData) const
{
  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)
  if (m_ResourceAccess.IsImmutable())
  {
    if (m_TextureFlags == ezGALTextureUsageFlags::ShaderResource)
    {
      if (initialData.IsEmpty())
      {
        ezLog::Error("Trying to create an immutable texture but not supplying initial data");
        return EZ_FAILURE;
      }
      if (initialData.GetCount() < m_uiMipLevelCount)
      {
        ezLog::Error("Trying to create an immutable texture but initialData size '{}' is smaller than mip levels '{}'", initialData.GetCount(), m_uiMipLevelCount);
        return EZ_FAILURE;
      }
    }
  }

  if (m_uiWidth == 0 || m_uiHeight == 0)
  {
    ezLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return EZ_FAILURE;
  }

  if (m_Type != ezGALTextureType::Texture2DArray && m_Type != ezGALTextureType::TextureCubeArray)
  {
    if (m_uiArraySize != 1)
    {
      ezLog::Error("m_uiArraySize must be 1 for non array textures!");
      return EZ_FAILURE;
    }
  }

  if (m_Format == ezGALResourceFormat::Invalid)
  {
    ezLog::Error("Texture format is 'Invalid'");
    return EZ_FAILURE;
  }

  const auto& caps = pDevice->GetCapabilities();
  ezBitflags<ezGALResourceFormatSupport> formatSupport = caps.m_FormatSupport[m_Format];
  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget) && !formatSupport.IsSet(ezGALResourceFormatSupport::RenderTarget))
  {
    ezLog::Error("ezGALTextureUsageFlags::RenderTarget not supported on format: {}", ezArgEnum(m_Format));
    return EZ_FAILURE;
  }
  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::UnorderedAccess) && !formatSupport.IsSet(ezGALResourceFormatSupport::TextureRW))
  {
    ezLog::Error("ezGALTextureUsageFlags::UnorderedAccess not supported on format: {}", ezArgEnum(m_Format));
    return EZ_FAILURE;
  }
  if (m_SampleCount == ezGALMSAASampleCount::TwoSamples && !formatSupport.IsSet(ezGALResourceFormatSupport::MSAA2x))
  {
    ezLog::Error("MSAA 2x not supported on format: {}", ezArgEnum(m_Format));
    return EZ_FAILURE;
  }
  if (m_SampleCount == ezGALMSAASampleCount::FourSamples && !formatSupport.IsSet(ezGALResourceFormatSupport::MSAA4x))
  {
    ezLog::Error("MSAA 4x not supported on format: {}", ezArgEnum(m_Format));
    return EZ_FAILURE;
  }
  if (m_SampleCount == ezGALMSAASampleCount::EightSamples && !formatSupport.IsSet(ezGALResourceFormatSupport::MSAA8x))
  {
    ezLog::Error("MSAA 8x not supported on format: {}", ezArgEnum(m_Format));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezUInt32 ezGALTextureCreationDescription::GetNumberOfSlices() const
{
  if (m_Type == ezGALTextureType::TextureCube || m_Type == ezGALTextureType::TextureCubeArray)
    return m_uiArraySize * 6;
  return m_uiArraySize;
}

ezVec3U32 ezGALTextureCreationDescription::GetMipMapSize(ezUInt32 uiMipLevel) const
{
  ezVec3U32 size = {m_uiWidth, m_uiHeight, m_uiDepth};
  size.x = ezMath::Max(1u, size.x >> uiMipLevel);
  size.y = ezMath::Max(1u, size.y >> uiMipLevel);
  size.z = ezMath::Max(1u, size.z >> uiMipLevel);
  return size;
}

ezBitflags<ezGALResourceState> ezGALTextureCreationDescription::GetDefaultState() const
{
  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::Presentable))
    return ezGALResourceState::Present;

  bool bDepth = ezGALResourceFormat::IsDepthFormat(m_Format);
  if (m_ResourceAccess.IsImmutable())
    return bDepth ? ezGALResourceState::DepthStencilRead : ezGALResourceState::ShaderResource;

  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::ShaderResource))
    return bDepth ? ezGALResourceState::DepthStencilRead : ezGALResourceState::ShaderResource;

  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
    return bDepth ? ezGALResourceState::DepthStencilWrite : ezGALResourceState::RenderTarget;

  if (m_TextureFlags.IsSet(ezGALTextureUsageFlags::UnorderedAccess))
    return ezGALResourceState::UnorderedAccess;

  return ezGALResourceState::Unknown;
}

ezBitflags<ezGALResourceState> ezGALBufferCreationDescription::GetDefaultState() const
{
  ezBitflags<ezGALResourceState> state = ezGALResourceState::Unknown;
  for (auto flag : m_BufferFlags)
  {
    switch (flag)
    {
      case ezGALBufferUsageFlags::VertexBuffer:
        state |= ezGALResourceState::VertexBuffer;
        break;
      case ezGALBufferUsageFlags::IndexBuffer:
        state |= ezGALResourceState::IndexBuffer;
        break;
      case ezGALBufferUsageFlags::ConstantBuffer:
        state |= ezGALResourceState::ConstantBuffer;
        break;
      case ezGALBufferUsageFlags::ShaderResource:
        state |= ezGALResourceState::ShaderResource;
        break;
      case ezGALBufferUsageFlags::UnorderedAccess:
        state |= ezGALResourceState::UnorderedAccess;
        break;
      case ezGALBufferUsageFlags::DrawIndirect:
        state |= ezGALResourceState::DrawIndirect;
        break;
      default:
        break;
    }
  }

  if (m_ResourceAccess.IsImmutable())
    return state & ezGALResourceState::AllReadStates;

  // This is the only write state for buffers. If set, all read states need to be removed to allow creating proper write -> read barriers.
  if (state.IsSet(ezGALResourceState::UnorderedAccess))
    state = ezGALResourceState::UnorderedAccess;

  return state;
}

ezUInt32 ezGALBindGroupLayoutCreationDescription::CalculateHash() const
{
  ezHashStreamWriter32 writer;
  auto HashBinding = [](ezHashStreamWriter32& writer, const ezShaderResourceBinding& binding)
  {
    writer << binding.m_ResourceType.GetValue();
    writer << binding.m_TextureType.GetValue();
    writer << binding.m_Stages.GetValue();
    writer << binding.m_iBindGroup;
    writer << binding.m_iSlot;
    writer << binding.m_uiArraySize;
    writer << binding.m_sName;
    if (binding.m_pLayout != nullptr)
    {
      const ezShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      writer << pLayout->m_uiTotalSize;
      for (const ezShaderConstant& constant : pLayout->m_Constants)
      {
        writer << constant.m_sName;
        writer << constant.m_Type.GetValue();
        writer << constant.m_uiArrayElements;
        writer << constant.m_uiOffset;
      }
    }
  };

  for (const ezShaderResourceBinding& binding : m_ResourceBindings)
  {
    HashBinding(writer, binding);
  }
  for (const ezShaderResourceBinding& binding : m_ImmutableSamplers)
  {
    HashBinding(writer, binding);
  }
  return writer.GetHashValue();
}
