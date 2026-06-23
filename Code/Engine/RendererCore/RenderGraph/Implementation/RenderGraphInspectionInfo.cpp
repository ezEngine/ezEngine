#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

#include <Foundation/IO/Stream.h>

namespace
{
  void WriteTextureRange(ezStreamWriter& inout_stream, const ezGALTextureRange& value)
  {
    inout_stream << value.m_uiBaseMipLevel;
    inout_stream << value.m_uiMipLevels;
    inout_stream << value.m_uiBaseArraySlice;
    inout_stream << value.m_uiArraySlices;
  }

  void ReadTextureRange(ezStreamReader& inout_stream, ezGALTextureRange& ref_value)
  {
    inout_stream >> ref_value.m_uiBaseMipLevel;
    inout_stream >> ref_value.m_uiMipLevels;
    inout_stream >> ref_value.m_uiBaseArraySlice;
    inout_stream >> ref_value.m_uiArraySlices;
  }

  void WriteTextureDesc(ezStreamWriter& inout_stream, const ezGALTextureCreationDescription& value)
  {
    inout_stream << value.m_uiWidth;
    inout_stream << value.m_uiHeight;
    inout_stream << value.m_uiDepth;
    inout_stream << value.m_uiArraySize;
    inout_stream << value.m_uiMipLevelCount;
    inout_stream << static_cast<ezUInt8>(value.m_Format.GetValue());
    inout_stream << static_cast<ezUInt8>(value.m_SampleCount.GetValue());
    inout_stream << static_cast<ezUInt8>(value.m_Type.GetValue());
    inout_stream << value.m_TextureFlags.GetValue();
    inout_stream << value.m_ResourceAccess.m_bImmutable;
    inout_stream << reinterpret_cast<ezUInt64>(value.m_pExisitingNativeObject);
  }

  void ReadTextureDesc(ezStreamReader& inout_stream, ezGALTextureCreationDescription& ref_value)
  {
    ezUInt8 uiFormat = 0;
    ezUInt8 uiSampleCount = 0;
    ezUInt8 uiType = 0;
    ezGALTextureUsageFlags::StorageType textureFlags = 0;
    ezUInt64 uiNativeObject = 0;

    inout_stream >> ref_value.m_uiWidth;
    inout_stream >> ref_value.m_uiHeight;
    inout_stream >> ref_value.m_uiDepth;
    inout_stream >> ref_value.m_uiArraySize;
    inout_stream >> ref_value.m_uiMipLevelCount;
    inout_stream >> uiFormat;
    inout_stream >> uiSampleCount;
    inout_stream >> uiType;
    inout_stream >> textureFlags;
    inout_stream >> ref_value.m_ResourceAccess.m_bImmutable;
    inout_stream >> uiNativeObject;

    ref_value.m_Format = static_cast<ezGALResourceFormat::Enum>(uiFormat);
    ref_value.m_SampleCount = static_cast<ezGALMSAASampleCount::Enum>(uiSampleCount);
    ref_value.m_Type = static_cast<ezGALTextureType::Enum>(uiType);
    ref_value.m_TextureFlags.SetValue(textureFlags);
    ref_value.m_pExisitingNativeObject = reinterpret_cast<void*>(uiNativeObject);
  }

  void WriteBufferDesc(ezStreamWriter& inout_stream, const ezGALBufferCreationDescription& value)
  {
    inout_stream << value.m_uiTotalSize;
    inout_stream << value.m_uiStructSize;
    inout_stream << value.m_BufferFlags.GetValue();
    inout_stream << value.m_ResourceAccess.m_bImmutable;
    inout_stream << static_cast<ezUInt8>(value.m_Format.GetValue());
  }

  void ReadBufferDesc(ezStreamReader& inout_stream, ezGALBufferCreationDescription& ref_value)
  {
    ezGALBufferUsageFlags::StorageType bufferFlags = 0;
    ezUInt8 uiFormat = 0;

    inout_stream >> ref_value.m_uiTotalSize;
    inout_stream >> ref_value.m_uiStructSize;
    inout_stream >> bufferFlags;
    inout_stream >> ref_value.m_ResourceAccess.m_bImmutable;
    inout_stream >> uiFormat;

    ref_value.m_BufferFlags.SetValue(bufferFlags);
    ref_value.m_Format = static_cast<ezGALResourceFormat::Enum>(uiFormat);
  }
} // namespace

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphObserverRequest& value)
{
  inout_stream << value.m_uiRenderGraphId;
  inout_stream << value.m_sPassName;
  inout_stream << value.m_uiAccessIndex;
  inout_stream << value.m_uiSwapChainId;
  inout_stream << value.m_uiArraySlice;
  inout_stream << value.m_uiMipLevel;
  inout_stream << value.m_uiChannelMask;
  inout_stream << value.m_iSampleIndex;
  inout_stream << value.m_fRangeMin;
  inout_stream << value.m_fRangeMax;
  inout_stream << value.m_fZoom;
  inout_stream << value.m_vPanCenter;
  inout_stream << value.m_vPixelPosition;
  inout_stream << value.m_bHighlightPixel;
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphObserverRequest& ref_value)
{
  inout_stream >> ref_value.m_uiRenderGraphId;
  inout_stream >> ref_value.m_sPassName;
  inout_stream >> ref_value.m_uiAccessIndex;
  inout_stream >> ref_value.m_uiSwapChainId;
  inout_stream >> ref_value.m_uiArraySlice;
  inout_stream >> ref_value.m_uiMipLevel;
  inout_stream >> ref_value.m_uiChannelMask;
  inout_stream >> ref_value.m_iSampleIndex;
  inout_stream >> ref_value.m_fRangeMin;
  inout_stream >> ref_value.m_fRangeMax;
  inout_stream >> ref_value.m_fZoom;
  inout_stream >> ref_value.m_vPanCenter;
  inout_stream >> ref_value.m_vPixelPosition;
  inout_stream >> ref_value.m_bHighlightPixel;
}

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphObserverResponse& value)
{
  inout_stream << value.m_fImageMin;
  inout_stream << value.m_fImageMax;
  inout_stream << static_cast<ezUInt32>(value.m_Histogram.GetCount());
  for (ezUInt8 uiValue : value.m_Histogram)
  {
    inout_stream << uiValue;
  }
  inout_stream << value.m_bHistogramValid;
  inout_stream << value.m_PixelValue;
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphObserverResponse& ref_value)
{
  ezUInt32 uiHistogramCount = 0;

  inout_stream >> ref_value.m_fImageMin;
  inout_stream >> ref_value.m_fImageMax;
  inout_stream >> uiHistogramCount;

  ref_value.m_Histogram.Clear();
  ref_value.m_Histogram.SetCount(ezMath::Min<ezUInt32>(uiHistogramCount, 1024));
  for (ezUInt32 i = 0; i < uiHistogramCount; ++i)
  {
    ezUInt8 uiValue = 0;
    inout_stream >> uiValue;
    if (i < ref_value.m_Histogram.GetCount())
    {
      ref_value.m_Histogram[i] = uiValue;
    }
  }

  inout_stream >> ref_value.m_bHistogramValid;
  inout_stream >> ref_value.m_PixelValue;
}

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphExecutionSummary& value)
{
  inout_stream << value.m_uiRenderGraphId;
  inout_stream << value.m_sGraphName;
  inout_stream << value.m_sUserName;
  inout_stream << static_cast<ezUInt8>(value.m_Phase.GetValue());
  inout_stream << value.m_uiExecutionOrder;
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphExecutionSummary& ref_value)
{
  ezUInt8 uiPhase = 0;

  inout_stream >> ref_value.m_uiRenderGraphId;
  inout_stream >> ref_value.m_sGraphName;
  inout_stream >> ref_value.m_sUserName;
  inout_stream >> uiPhase;
  inout_stream >> ref_value.m_uiExecutionOrder;

  ref_value.m_Phase = static_cast<ezRenderGraphPhase::Enum>(uiPhase);
}

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphSwapChainSummary& value)
{
  inout_stream << value.m_uiSwapChainId;
  inout_stream << value.m_uiWidth;
  inout_stream << value.m_uiHeight;
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphSwapChainSummary& ref_value)
{
  inout_stream >> ref_value.m_uiSwapChainId;
  inout_stream >> ref_value.m_uiWidth;
  inout_stream >> ref_value.m_uiHeight;
}

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphInspectionSummary& value)
{
  inout_stream << static_cast<ezUInt32>(value.m_RenderGraphs.GetCount());
  for (const auto& graph : value.m_RenderGraphs)
  {
    inout_stream << graph;
  }

  inout_stream << static_cast<ezUInt32>(value.m_AvailableSwapChains.GetCount());
  for (const auto& swapChain : value.m_AvailableSwapChains)
  {
    inout_stream << swapChain;
  }
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphInspectionSummary& ref_value)
{
  ezUInt32 uiGraphCount = 0;
  ezUInt32 uiSwapChainCount = 0;

  inout_stream >> uiGraphCount;
  ref_value.m_RenderGraphs.SetCount(uiGraphCount);
  for (auto& graph : ref_value.m_RenderGraphs)
  {
    inout_stream >> graph;
  }

  inout_stream >> uiSwapChainCount;
  ref_value.m_AvailableSwapChains.SetCount(uiSwapChainCount);
  for (auto& swapChain : ref_value.m_AvailableSwapChains)
  {
    inout_stream >> swapChain;
  }
}

void operator<<(ezStreamWriter& inout_stream, const ezRenderGraphInspectionInfo& value)
{
  inout_stream << static_cast<ezUInt32>(value.m_Passes.GetCount());
  for (const auto& pass : value.m_Passes)
  {
    inout_stream << pass.m_sName;
    inout_stream << static_cast<ezUInt8>(pass.m_QueueType.GetValue());
    inout_stream << pass.m_bHasSideEffects;
    inout_stream << pass.m_bAlive;
  }

  inout_stream << static_cast<ezUInt32>(value.m_Textures.GetCount());
  for (const auto& texture : value.m_Textures)
  {
    WriteTextureDesc(inout_stream, texture.m_Desc);
    inout_stream << texture.m_bImported;
    inout_stream << texture.m_uiFirstUsePassIndex;
    inout_stream << texture.m_uiLastUsePassIndex;
    inout_stream << texture.m_uiResolvedIndex;
  }

  inout_stream << static_cast<ezUInt32>(value.m_Buffers.GetCount());
  for (const auto& buffer : value.m_Buffers)
  {
    WriteBufferDesc(inout_stream, buffer.m_Desc);
    inout_stream << buffer.m_bImported;
    inout_stream << buffer.m_uiFirstUsePassIndex;
    inout_stream << buffer.m_uiLastUsePassIndex;
    inout_stream << buffer.m_uiResolvedIndex;
  }

  inout_stream << static_cast<ezUInt32>(value.m_Accesses.GetCount());
  for (const auto& access : value.m_Accesses)
  {
    inout_stream << access.m_uiPassIndex;
    inout_stream << access.m_uiResourceIndex;
    inout_stream << access.m_uiAccessIndex;
    inout_stream << access.m_bIsTexture;
    inout_stream << access.m_Access.GetValue();
    WriteTextureRange(inout_stream, access.m_TextureRange);
  }
}

void operator>>(ezStreamReader& inout_stream, ezRenderGraphInspectionInfo& ref_value)
{
  ezUInt32 uiCount = 0;

  inout_stream >> uiCount;
  ref_value.m_Passes.SetCount(uiCount);
  for (auto& pass : ref_value.m_Passes)
  {
    ezUInt8 uiQueueType = 0;
    inout_stream >> pass.m_sName;
    inout_stream >> uiQueueType;
    inout_stream >> pass.m_bHasSideEffects;
    inout_stream >> pass.m_bAlive;
    pass.m_QueueType = static_cast<ezGALQueueType::Enum>(uiQueueType);
  }

  inout_stream >> uiCount;
  ref_value.m_Textures.SetCount(uiCount);
  for (auto& texture : ref_value.m_Textures)
  {
    ReadTextureDesc(inout_stream, texture.m_Desc);
    inout_stream >> texture.m_bImported;
    inout_stream >> texture.m_uiFirstUsePassIndex;
    inout_stream >> texture.m_uiLastUsePassIndex;
    inout_stream >> texture.m_uiResolvedIndex;
  }

  inout_stream >> uiCount;
  ref_value.m_Buffers.SetCount(uiCount);
  for (auto& buffer : ref_value.m_Buffers)
  {
    ReadBufferDesc(inout_stream, buffer.m_Desc);
    inout_stream >> buffer.m_bImported;
    inout_stream >> buffer.m_uiFirstUsePassIndex;
    inout_stream >> buffer.m_uiLastUsePassIndex;
    inout_stream >> buffer.m_uiResolvedIndex;
  }

  inout_stream >> uiCount;
  ref_value.m_Accesses.SetCount(uiCount);
  for (auto& access : ref_value.m_Accesses)
  {
    ezGALResourceState::StorageType accessFlags = 0;
    inout_stream >> access.m_uiPassIndex;
    inout_stream >> access.m_uiResourceIndex;
    inout_stream >> access.m_uiAccessIndex;
    inout_stream >> access.m_bIsTexture;
    inout_stream >> accessFlags;
    access.m_Access.SetValue(accessFlags);
    ReadTextureRange(inout_stream, access.m_TextureRange);
  }
}

void ezRenderGraphInspectionInfo::Swap(ezRenderGraphInspectionInfo& ref_data)
{
  m_Passes.Swap(ref_data.m_Passes);
  m_Textures.Swap(ref_data.m_Textures);
  m_Buffers.Swap(ref_data.m_Buffers);
  m_Accesses.Swap(ref_data.m_Accesses);
}

void ezRenderGraphInspectionInfo::Clear()
{
  m_Passes.Clear();
  m_Textures.Clear();
  m_Buffers.Clear();
  m_Accesses.Clear();
}
