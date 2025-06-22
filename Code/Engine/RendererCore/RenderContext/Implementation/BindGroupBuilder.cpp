

#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererCore/RenderContext/BindGroupBuilder.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RendererFallbackResources.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

ezUInt32 ezBindGroupBuilder::s_uiWrites = 0;
ezUInt32 ezBindGroupBuilder::s_uiReads = 0;

ezBindGroupBuilder::ezBindGroupBuilder()
{
  // Platforms that do not support immutable samples like DX11 still need them to be bound manually, so they are bound here.
  ezTempHashedString sLinearSampler("LinearSampler");
  for (auto it : ezGALImmutableSamplers::GetImmutableSamplers())
  {
    ezGALBindGroupItem item;
    item.m_Flags = ezGALBindGroupItemFlags::Sampler;
    item.m_Sampler.m_hSampler = it.Value();
    m_BoundSamplers.Insert(it.Key().GetHash(), item);

    if (it.Key() == sLinearSampler)
    {
      m_hDefaultSampler = it.Value();
    }
  }
  EZ_ASSERT_DEBUG(!m_hDefaultSampler.IsInvalidated(), "LinearSampler should have been registered at this point.");
}

void ezBindGroupBuilder::BindSampler(const ezTempHashedString& sSlotName, ezGALSamplerStateHandle hSampler)
{
  EZ_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  if (hSampler.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundSamplers);
    return;
  }

  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Sampler;
  item.m_Sampler.m_hSampler = hSampler;

  InsertItem(sSlotName, item, m_BoundSamplers);
}

void ezBindGroupBuilder::BindBuffer(const ezTempHashedString& sSlotName, ezGALBufferHandle hBuffer, ezGALBufferRange bufferRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hBuffer.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundBuffers);
    return;
  }

  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Buffer;
  item.m_Buffer.m_hBuffer = hBuffer;
  item.m_Buffer.m_BufferRange = bufferRange;
  item.m_Buffer.m_OverrideViewFormat = overrideViewFormat;

  InsertItem(sSlotName, item, m_BoundBuffers);
}

void ezBindGroupBuilder::BindTexture(const ezTempHashedString& sSlotName, ezGALTextureHandle hTexture, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hTexture.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundTextures);
    return;
  }

  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Texture;
  item.m_Texture.m_hTexture = hTexture;
  item.m_Texture.m_hSampler = {};
  item.m_Texture.m_TextureRange = textureRange;
  item.m_Texture.m_OverrideViewFormat = overrideViewFormat;

  InsertItem(sSlotName, item, m_BoundTextures);
}

void ezBindGroupBuilder::BindTexture(const ezTempHashedString& sSlotName, const ezTexture2DResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTexture2DResource> pTexture(hTexture, acquireMode);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat);
    BindSampler(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void ezBindGroupBuilder::BindTexture(const ezTempHashedString& sSlotName, const ezTexture3DResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat);
    BindSampler(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void ezBindGroupBuilder::BindTexture(const ezTempHashedString& sSlotName, const ezTextureCubeResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hTexture.IsValid())
  {
    ezResourceLock<ezTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTexture(sSlotName, pTexture->GetGALTexture(), textureRange, overrideViewFormat);
    BindSampler(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture(sSlotName, {}, textureRange, overrideViewFormat);
    BindSampler(sSlotName, {});
  }
}

void ezBindGroupBuilder::BindBuffer(const ezTempHashedString& sSlotName, ezConstantBufferStorageHandle hBuffer, ezGALBufferRange bufferRange, ezGALResourceFormat::Enum overrideViewFormat)
{
  ezConstantBufferStorageBase* pStorage = nullptr;
  if (ezRenderContext::TryGetConstantBufferStorage(hBuffer, pStorage))
  {
    BindBuffer(sSlotName, pStorage->GetGALBufferHandle(), bufferRange, overrideViewFormat);
  }
  else
  {
    BindBuffer(sSlotName, ezGALBufferHandle(), bufferRange, overrideViewFormat);
  }
}

ezResult ezBindGroupBuilder::CreateBindGroup(const ezGALDevice* pDevice, ezGALBindGroupLayoutHandle hBindGroupLayout, ezGALBindGroupCreationDescription& out_BindGroup)
{
  EZ_LOG_BLOCK("CreateBindGroup");
  const ezGALBindGroupLayout* pLayout = pDevice->GetBindGroupLayout(hBindGroupLayout);
  if (pLayout == nullptr)
  {
    ezLog::Error("Bind group layout is null.");
    return EZ_FAILURE;
  }
  const ezArrayPtr<const ezShaderResourceBinding> resourceBindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = resourceBindings.GetCount();
  out_BindGroup.m_hBindGroupLayout = hBindGroupLayout;
  out_BindGroup.m_BindGroupItems.Clear();
  out_BindGroup.m_BindGroupItems.Reserve(uiBindings);

  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = resourceBindings[i];
    ezGALBindGroupItem& item = out_BindGroup.m_BindGroupItems.ExpandAndGetRef();

    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::Sampler:
      {
        s_uiReads++;
        if (!m_BoundSamplers.TryGetValue(binding.m_sName.GetHash(), item))
        {
          s_uiReads++;
          item.m_Flags = ezGALBindGroupItemFlags::Sampler;
          item.m_Sampler.m_hSampler = m_hDefaultSampler;
        }
      }
      break;
      case ezGALShaderResourceType::ConstantBuffer:
      case ezGALShaderResourceType::TexelBuffer:
      case ezGALShaderResourceType::StructuredBuffer:
      case ezGALShaderResourceType::ByteAddressBuffer:
      case ezGALShaderResourceType::TexelBufferRW:
      case ezGALShaderResourceType::StructuredBufferRW:
      case ezGALShaderResourceType::ByteAddressBufferRW:
      {
        s_uiReads++;
        if (!m_BoundBuffers.TryGetValue(binding.m_sName.GetHash(), item))
        {
          if (ezGALBufferHandle hBuffer = ezGALRendererFallbackResources::GetFallbackBuffer(binding.m_ResourceType); !hBuffer.IsInvalidated())
          {
            item.m_Flags = ezGALBindGroupItemFlags::Buffer | ezGALBindGroupItemFlags::Fallback;
            item.m_Buffer.m_hBuffer = hBuffer;
            item.m_Buffer.m_BufferRange = {};
            item.m_Buffer.m_OverrideViewFormat = {};
          }
          else
          {
            ezLog::Error("No buffer bound for binding '{0}'", binding.m_sName);
            return EZ_FAILURE;
          }
        }

        const ezGALBuffer* pBuffer = pDevice->GetBuffer(item.m_Buffer.m_hBuffer);
        if (pBuffer == nullptr)
          return EZ_FAILURE;
        if (item.m_Buffer.m_BufferRange.m_uiByteCount == EZ_GAL_WHOLE_SIZE)
        {
          item.m_Buffer.m_BufferRange.m_uiByteCount = pBuffer->GetDescription().m_uiTotalSize - item.m_Buffer.m_BufferRange.m_uiByteOffset;
        }
      }
      break;

      case ezGALShaderResourceType::Texture:
      case ezGALShaderResourceType::TextureRW:
      case ezGALShaderResourceType::TextureAndSampler:
      {
        s_uiReads++;
        if (!m_BoundTextures.TryGetValue(binding.m_sName.GetHash(), item))
        {
          const bool bDepth = binding.m_sName.GetString().FindSubString_NoCase("shadow") != nullptr || binding.m_sName.GetString().FindSubString_NoCase("depth");
          if (ezGALTextureHandle hTexture = ezGALRendererFallbackResources::GetFallbackTexture(binding.m_ResourceType, binding.m_TextureType, bDepth); !hTexture.IsInvalidated())
          {
            item.m_Flags = ezGALBindGroupItemFlags::Texture | ezGALBindGroupItemFlags::Fallback;
            item.m_Texture.m_hTexture = hTexture;
            item.m_Texture.m_hSampler = {};
            item.m_Texture.m_TextureRange = {};
            item.m_Texture.m_OverrideViewFormat = {};
          }
          else
          {
            ezLog::Error("No buffer bound for binding '{0}'", binding.m_sName);
            return EZ_FAILURE;
          }
        }

        if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
        {
          s_uiReads++;
          const ezGALBindGroupItem* pSamplerItem = nullptr;
          if (!m_BoundSamplers.TryGetValue(binding.m_sName.GetHash(), pSamplerItem))
          {
            item.m_Texture.m_hSampler = m_hDefaultSampler;
          }
          else
          {
            item.m_Texture.m_hSampler = pSamplerItem->m_Sampler.m_hSampler;
          }
        }

        if (!ezGALShaderTextureType::IsArray(binding.m_TextureType))
        {
          item.m_Texture.m_TextureRange.m_uiArraySlices = binding.m_TextureType == ezGALShaderTextureType::TextureCube ? 6 : 1;
        }
        const ezGALTexture* pTexture = pDevice->GetTexture(item.m_Texture.m_hTexture);
        if (pTexture == nullptr)
          return EZ_FAILURE;

        // Resolve proxy texture as they only cause pain down the pipeline.
        if (pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy)
        {
          const auto pProxy = static_cast<const ezGALProxyTexture*>(pTexture);
          item.m_Texture.m_hTexture = pProxy->GetParentTextureHandle();
          item.m_Texture.m_TextureRange.m_uiBaseArraySlice = pProxy->GetSlice();
          item.m_Texture.m_TextureRange.m_uiArraySlices = 1;
          pTexture = static_cast<const ezGALTexture*>(pProxy->GetParentResource());
        }
        item.m_Texture.m_TextureRange = pTexture->ClampRange(item.m_Texture.m_TextureRange);
      }
      break;
      case ezGALShaderResourceType::PushConstants:
      case ezGALShaderResourceType::Unknown:
      default:
        ezLog::Error("Unsupported resource type for binding '{0}'", binding.m_sName, binding.m_ResourceType);
        return EZ_FAILURE;
        break;
    }
  }
  m_bModified = false;
  return EZ_SUCCESS;
}

void ezBindGroupBuilder::RemoveItem(const ezTempHashedString& sSlotName, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  if (ref_Container.Contains(sSlotName.GetHash()))
  {
    ref_Container.Remove(sSlotName.GetHash());
    m_bModified = true;
    s_uiWrites++;
  }
}

void ezBindGroupBuilder::InsertItem(const ezTempHashedString& sSlotName, const ezGALBindGroupItem& item, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  ezGALBindGroupItem* pOldItem = nullptr;
  if (ref_Container.TryGetValue(sSlotName.GetHash(), pOldItem))
  {
    if (*pOldItem == item)
    {
      return;
    }
  }

  ref_Container.Insert(sSlotName.GetHash(), item);
  m_bModified = true;
  s_uiWrites++;
}
