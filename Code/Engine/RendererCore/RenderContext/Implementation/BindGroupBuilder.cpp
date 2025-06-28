

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

ezBindGroupBuilder::ezBindGroupBuilder() = default;


void ezBindGroupBuilder::ResetBoundResources(const ezGALDevice* pDevice)
{
  m_pDevice = pDevice;
  m_bModified = true;
  m_hDefaultSampler = {};
  m_BoundSamplers.Clear();
  m_BoundBuffers.Clear();
  m_BoundTextures.Clear();

  // Platforms that do not support immutable samplers like DX11 still need them to be bound manually, so they are bound here.
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

void ezBindGroupBuilder::BindSampler(ezTempHashedString sSlotName, ezGALSamplerStateHandle hSampler)
{
  EZ_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a reserved sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a reserved sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a reserved sampler name and must not be set manually.");
  EZ_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a reserved sampler name and must not be set manually.");

  if (hSampler.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundSamplers);
    return;
  }

  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Sampler;
  item.m_Sampler.m_hSampler = hSampler;
  EZ_ASSERT_DEBUG(m_pDevice->GetSamplerState(item.m_Sampler.m_hSampler) != nullptr, "Invalid sampler handle bound.");

  InsertItem(sSlotName, item, m_BoundSamplers);
}

void ezBindGroupBuilder::BindBuffer(ezTempHashedString sSlotName, ezGALBufferHandle hBuffer, ezGALBufferRange bufferRange, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat)
{
  if (hBuffer.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundBuffers);
    return;
  }

  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  EZ_ASSERT_DEBUG(pBuffer != nullptr, "Invalid buffer handle bound.");
  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Buffer;
  item.m_Buffer.m_hBuffer = hBuffer;
  item.m_Buffer.m_BufferRange = pBuffer->ClampRange(bufferRange);
  item.m_Buffer.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  InsertItem(sSlotName, item, m_BoundBuffers);
}

void ezBindGroupBuilder::BindTexture(ezTempHashedString sSlotName, ezGALTextureHandle hTexture, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
{
  if (hTexture.IsInvalidated())
  {
    RemoveItem(sSlotName, m_BoundTextures);
    return;
  }

  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  EZ_ASSERT_DEBUG(pTexture != nullptr, "Invalid texture handle bound.");
  // Resolve proxy texture as they only cause pain down the pipeline.
  if (pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy)
  {
    const auto pProxy = static_cast<const ezGALProxyTexture*>(pTexture);
    hTexture = pProxy->GetParentTextureHandle();
    pTexture = static_cast<const ezGALTexture*>(pProxy->GetParentResource());
    textureRange = {pProxy->GetSlice(), 1, 0, EZ_GAL_ALL_MIP_LEVELS};
  }

  ezGALBindGroupItem item;
  item.m_Flags = ezGALBindGroupItemFlags::Texture;
  item.m_Texture.m_hTexture = hTexture;
  item.m_Texture.m_hSampler = {};
  item.m_Texture.m_TextureRange = pTexture->ClampRange(textureRange);
  item.m_Texture.m_OverrideViewFormat = overrideViewFormat;

  InsertItem(sSlotName, item, m_BoundTextures);
}

void ezBindGroupBuilder::BindTexture(ezTempHashedString sSlotName, const ezTexture2DResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
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

void ezBindGroupBuilder::BindTexture(ezTempHashedString sSlotName, const ezTexture3DResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
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

void ezBindGroupBuilder::BindTexture(ezTempHashedString sSlotName, const ezTextureCubeResourceHandle& hTexture, ezResourceAcquireMode acquireMode, ezGALTextureRange textureRange, ezEnum<ezGALResourceFormat> overrideViewFormat)
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

void ezBindGroupBuilder::BindBuffer(ezTempHashedString sSlotName, ezConstantBufferStorageHandle hBuffer, ezGALBufferRange bufferRange, ezGALResourceFormat::Enum overrideTexelBufferFormat)
{
  ezConstantBufferStorageBase* pStorage = nullptr;
  if (ezRenderContext::TryGetConstantBufferStorage(hBuffer, pStorage))
  {
    BindBuffer(sSlotName, pStorage->GetGALBufferHandle(), bufferRange, overrideTexelBufferFormat);
  }
  else
  {
    BindBuffer(sSlotName, ezGALBufferHandle(), bufferRange, overrideTexelBufferFormat);
  }
}

void ezBindGroupBuilder::CreateBindGroup(ezGALBindGroupLayoutHandle hBindGroupLayout, ezGALBindGroupCreationDescription& out_bindGroup)
{
  const ezGALBindGroupLayout* pLayout = m_pDevice->GetBindGroupLayout(hBindGroupLayout);
  EZ_ASSERT_DEBUG(pLayout != nullptr, "Bind group layout is null.");

  const ezArrayPtr<const ezShaderResourceBinding> resourceBindings = pLayout->GetDescription().m_ResourceBindings;
  const ezUInt32 uiBindings = resourceBindings.GetCount();
  out_bindGroup.m_hBindGroupLayout = hBindGroupLayout;
  out_bindGroup.m_BindGroupItems.Clear();
  out_bindGroup.m_BindGroupItems.Reserve(uiBindings);

  for (ezUInt32 i = 0; i < uiBindings; ++i)
  {
    const ezShaderResourceBinding& binding = resourceBindings[i];
    ezGALBindGroupItem& item = out_bindGroup.m_BindGroupItems.ExpandAndGetRef();

    switch (binding.m_ResourceType)
    {
      case ezGALShaderResourceType::Sampler:
      {
        s_uiReads++;
        if (!m_BoundSamplers.TryGetValue(binding.m_sName.GetHash(), item))
        {
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
          ezGALBufferHandle hBuffer = ezGALRendererFallbackResources::GetFallbackBuffer(binding.m_ResourceType);
          EZ_ASSERT_DEBUG(!hBuffer.IsInvalidated(), "Missing fallback resource for binding resource type {}", binding.m_ResourceType.GetValue());
          const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
          item.m_Flags = ezGALBindGroupItemFlags::Buffer | ezGALBindGroupItemFlags::Fallback;
          item.m_Buffer.m_hBuffer = hBuffer;
          item.m_Buffer.m_BufferRange = pBuffer->ClampRange({});
          ;
          item.m_Buffer.m_OverrideTexelBufferFormat = {};
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
          ezGALTextureHandle hTexture = ezGALRendererFallbackResources::GetFallbackTexture(binding.m_ResourceType, binding.m_TextureType, bDepth);
          EZ_ASSERT_DEBUG(!hTexture.IsInvalidated(), "Missing fallback resource for binding resource type {}, texture type {}, depth {}", binding.m_ResourceType.GetValue(), binding.m_TextureType.GetValue(), bDepth);
          const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
          item.m_Flags = ezGALBindGroupItemFlags::Texture | ezGALBindGroupItemFlags::Fallback;
          item.m_Texture.m_hTexture = hTexture;
          item.m_Texture.m_hSampler = {};
          item.m_Texture.m_TextureRange = pTexture->ClampRange({});
          item.m_Texture.m_OverrideViewFormat = {};
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
      }
      break;
      case ezGALShaderResourceType::PushConstants:
      case ezGALShaderResourceType::Unknown:
      default:
        EZ_REPORT_FAILURE("Unsupported resource type for binding '{0}'", binding.m_sName, binding.m_ResourceType);
        break;
    }
  }
  m_bModified = false;
}

void ezBindGroupBuilder::RemoveItem(ezTempHashedString sSlotName, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  if (ref_Container.Remove(sSlotName.GetHash()))
  {
    m_bModified = true;
    s_uiWrites++;
  }
}

void ezBindGroupBuilder::InsertItem(ezTempHashedString sSlotName, const ezGALBindGroupItem& item, ezHashTable<ezUInt64, ezGALBindGroupItem>& ref_Container)
{
  s_uiReads++;
  ezGALBindGroupItem oldItem;
  if (ref_Container.Insert(sSlotName.GetHash(), item, &oldItem))
  {
    if (oldItem != item)
    {
      m_bModified = true;
      s_uiWrites++;
    }
  }
  else
  {
    m_bModified = true;
    s_uiWrites++;
  }
}
