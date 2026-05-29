#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>

namespace
{
  bool HasOneBitSet(ezUInt32 x)
  {
    return x && !(x & (x - 1));
  }

  void ClampRenderTarget(ezGALRenderTargetRange& ref_range, const ezGALTextureCreationDescription& desc)
  {
    if (ref_range.m_uiArraySlices == EZ_GAL_ALL_ARRAY_SLICES)
    {
      ref_range.m_uiArraySlices = desc.m_uiArraySize;
    }
  }

  // We encode a full resource rage as the default constructed ezGALTextureRange to allow us to quickly categorize full resources barriers vs sub-resource barriers without having to lookup the resource description every time.
  void MakeFullRange(ezGALTextureRange& ref_range, const ezGALTextureCreationDescription& desc)
  {
    bool bAllSlices = (ref_range.m_uiBaseArraySlice == 0 && (ref_range.m_uiArraySlices == EZ_GAL_ALL_ARRAY_SLICES || ref_range.m_uiArraySlices == desc.m_uiArraySize));
    bool bAllMips = (ref_range.m_uiBaseMipLevel == 0 && (ref_range.m_uiArraySlices == EZ_GAL_ALL_MIP_LEVELS || ref_range.m_uiArraySlices == desc.m_uiMipLevelCount));
    if (bAllSlices && bAllMips)
    {
      ref_range = ezGALTextureRange{};
    }
  }

  bool MergeRanges(ezGALTextureRange& ref_range, ezBitflags<ezGALResourceState>& ref_access, ezBitflags<ezGALShaderStageFlags>& ref_stage, const ezGALTextureRange& range, ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
  {
    // Write states are exclusive and can't be merged
    if ((ref_access.IsAnySet(ezGALResourceState::AllWriteStates) || access.IsAnySet(ezGALResourceState::AllWriteStates)) && ref_access != access)
      return false;

    if (ref_range.m_uiBaseArraySlice != range.m_uiBaseArraySlice || ref_range.m_uiArraySlices != range.m_uiArraySlices || ref_range.m_uiBaseMipLevel != range.m_uiBaseMipLevel || ref_range.m_uiMipLevels != range.m_uiMipLevels)
      return false;

    ref_access |= access;
    ref_stage |= stage;
    return true;
  }
} // namespace

ezRenderGraphPassBuilder::ezRenderGraphPassBuilder(ezRenderGraph* pParent)
  : m_pParent(pParent)
{
}

ezRenderGraphPassBuilder::ezRenderGraphPassBuilder(ezRenderGraphPassBuilder&& rhs) noexcept
{
  *this = std::move(rhs);
}

ezRenderGraphPassBuilder::~ezRenderGraphPassBuilder()
{
  if (m_pParent != nullptr)
  {
    // Flush render target / depth target writes / reads
    // This is deferred to here so that the load / store ops can be modified by SetClearColor etc. Note this will be needed once we can represent a barrier that discards the old state (triggered by ezGALRenderTargetLoadOp::DontCare). For now, all barriers assume `Load` for simplicity.
    const ezUInt16 uiColorTargetIndex = m_pParent->m_pCurrentPass->m_uiColorTargetIndex;
    const ezUInt8 uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
    for (ezUInt8 i = 0; i < uiColorTargetCount; ++i)
    {
      const ezRenderGraph::ColorTargetInfo& info = m_pParent->m_ColorTargets[uiColorTargetIndex + i];
      ezBitflags<ezGALResourceState> state = ezGALResourceState::RenderTarget;
      if (info.m_loadOp != ezGALRenderTargetLoadOp::Load)
        state |= ezGALResourceState::Discard;
      WriteTexture(info.m_hTexture, ezGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
    }

    const ezUInt16 uiDepthTargetIndex = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex;
    const ezUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
    for (ezUInt8 i = 0; i < uiDepthTargetCount; ++i)
    {
      const ezRenderGraph::DepthStencilTargetInfo& info = m_pParent->m_DepthStencilTargets[uiDepthTargetIndex + i];
      ezBitflags<ezGALResourceState> state = info.m_bReadOnly ? ezGALResourceState::DepthStencilRead : ezGALResourceState::DepthStencilWrite;
      const bool bStencil = ezGALResourceFormat::IsStencilFormat(m_pParent->GetTextureDesc(info.m_hTexture).m_Format);
      if (info.m_depthLoadOp != ezGALRenderTargetLoadOp::Load && !bStencil || info.m_stencilLoadOp != ezGALRenderTargetLoadOp::Load)
        state |= ezGALResourceState::Discard;
      if (info.m_bReadOnly)
      {
        ReadTexture(info.m_hTexture, ezGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
      }
      else
      {
        WriteTexture(info.m_hTexture, ezGALTextureRange::MakeFromRenderTargetRange(info.m_range), state);
      }
    }

    m_pParent->EndPassBuilder();
  }
}

void ezRenderGraphPassBuilder::operator=(ezRenderGraphPassBuilder&& rhs) noexcept
{
  if (m_pParent)
  {
    m_pParent->EndPassBuilder();
  }
  m_pParent = rhs.m_pParent;
  rhs.m_pParent = nullptr;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::ReadTexture(ezRenderGraphTextureHandle hTexture,
  ezGALTextureRange range, ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(HasOneBitSet(access.GetValue()), "ReadTexture: Exactly one state must be set");
  if (m_pParent->m_pCurrentPass->m_QueueType == ezGALQueueType::Compute && stage == ezGALShaderStageFlags::Auto)
    stage = ezGALShaderStageFlags::ComputeShader;

  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllTextureStates) == access, "ReadTexture: Only texture states are allowed");
  EZ_ASSERT_DEBUG(hTexture.GetInternalID().m_InstanceIndex < m_pParent->m_TextureCreationDescriptions.GetCount(), "ReadTexture: hTexture is invalid");
  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllReadStates) == access, "ReadTexture: Only read states are allowed");
  ezGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  MakeFullRange(range, desc);

  // Convenience functionality. We still need the differentiation in the low level renderer but here we can just patch it.
  if (access.IsSet(ezGALResourceState::ShaderResource) && ezGALResourceFormat::IsDepthFormat(desc.m_Format))
  {
    access.Remove(ezGALResourceState::ShaderResource);
    access.Add(ezGALResourceState::DepthStencilRead);
  }

  const bool bIsImported = m_pParent->m_HandleToImportTexture.Contains(hTexture);
  if (bIsImported)
  {
    // Verify imported texture supports operation
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::ShaderResource:
        case ezGALResourceState::DepthStencilRead:
          EZ_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::ShaderResource), "ShaderResource flag required on imported texture, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp texture
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::ShaderResource:
        case ezGALResourceState::DepthStencilRead:
          if (!desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::ShaderResource))
          {
            desc.m_TextureFlags |= ezGALTextureUsageFlags::ShaderResource;
            EZ_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
        default:
          break;
      }
    }
  }

  ezArrayPtr<const ezRenderGraph::TextureInfo> reads = m_pParent->m_pCurrentPass->GetReadTextures(m_pParent);
  for (ezUInt32 uiIndex = 0; uiIndex < reads.GetCount(); ++uiIndex)
  {
    // Cast away const so we can merge into the existing entry in-place.
    auto& readInfo = const_cast<ezRenderGraph::TextureInfo&>(reads[uiIndex]);
    if (readInfo.m_hTexture != hTexture)
      continue;

    if (!range.Overlaps(readInfo.m_range))
      continue;

    if (MergeRanges(readInfo.m_range, readInfo.m_access, readInfo.m_stage, range, access, stage))
      return *this;

    EZ_REPORT_FAILURE("ReadTexture: Overlapping read-write conflict on the same texture within a single pass");
  }

  m_pParent->m_ReadTextures.PushBack({hTexture, range, stage, access});
  m_pParent->m_pCurrentPass->m_uiReadTextureCount++;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::WriteTexture(ezRenderGraphTextureHandle hTexture,
  ezGALTextureRange range, ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(HasOneBitSet(access.GetValue() & ~ezGALResourceState::Discard), "WriteTexture: Exactly one state must be set");
  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllTextureStates) == access, "WriteTexture: Only texture states are allowed");
  EZ_ASSERT_DEBUG(hTexture.GetInternalID().m_InstanceIndex < m_pParent->m_TextureCreationDescriptions.GetCount(), "WriteTexture: hTexture is invalid");
  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllWriteStates) == access, "WriteTexture: Only write states are allowed");
  ezGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  MakeFullRange(range, desc);

  const bool bIsImported = m_pParent->m_HandleToImportTexture.Contains(hTexture);
  if (bIsImported)
  {
    EZ_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't write to immutable textures");
    // Verify imported texture supports operation
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::UnorderedAccess:
          EZ_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::UnorderedAccess), "UnorderedAccess flag required on imported texture, operation invalid");
          break;
        case ezGALResourceState::RenderTarget:
        case ezGALResourceState::DepthStencilWrite:
          EZ_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget), "RenderTarget flag required on imported texture, operation invalid");
          break;
        case ezGALResourceState::Present:
          EZ_ASSERT_DEBUG(desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::Presentable), "Presentable flag required on imported texture, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp texture
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::UnorderedAccess:
          if (!desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::UnorderedAccess))
          {
            desc.m_TextureFlags |= ezGALTextureUsageFlags::UnorderedAccess;
            EZ_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
          break;
        case ezGALResourceState::RenderTarget:
        case ezGALResourceState::DepthStencilWrite:
          if (!desc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
          {
            desc.m_TextureFlags |= ezGALTextureUsageFlags::RenderTarget;
            EZ_ASSERT_DEBUG(desc.Validate(m_pParent->m_pDevice).Succeeded(), "Invalid texture desc");
          }
          break;
        case ezGALResourceState::Present:
          desc.m_TextureFlags |= ezGALTextureUsageFlags::Presentable;
          break;
        default:
          break;
      }
    }
  }

  ezArrayPtr<const ezRenderGraph::TextureInfo> writes = m_pParent->m_pCurrentPass->GetWriteTextures(m_pParent);
  for (ezUInt32 uiIndex = 0; uiIndex < writes.GetCount(); ++uiIndex)
  {
    // Cast away const so we can merge into the existing entry in-place.
    auto& writeInfo = const_cast<ezRenderGraph::TextureInfo&>(writes[uiIndex]);
    if (writeInfo.m_hTexture != hTexture)
      continue;

    if (!range.Overlaps(writeInfo.m_range))
      continue;

    if (MergeRanges(writeInfo.m_range, writeInfo.m_access, writeInfo.m_stage, range, access, stage))
      return *this;

    EZ_REPORT_FAILURE("WriteTexture: Overlapping write conflict on the same texture within a single pass");
  }

  m_pParent->m_WriteTextures.PushBack({hTexture, range, stage, access});
  m_pParent->m_pCurrentPass->m_uiWriteTextureCount++;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::ReadBuffer(ezRenderGraphBufferHandle hBuffer,
  ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllBufferStates) == access, "ReadBuffer: Only buffer states are allowed");
  EZ_ASSERT_DEBUG(hBuffer.GetInternalID().m_InstanceIndex < m_pParent->m_BufferCreationDescriptions.GetCount(), "ReadBuffer: hBuffer is invalid");
  EZ_ASSERT_DEBUG(access.IsAnySet(ezGALResourceState::AllReadStates), "ReadBuffer: Only read states are allowed");
  ezGALBufferCreationDescription& desc = m_pParent->m_BufferCreationDescriptions[hBuffer.GetInternalID().m_InstanceIndex];

  const bool bIsImported = m_pParent->m_HandleToImportBuffer.Contains(hBuffer);
  if (bIsImported)
  {
    // Verify imported buffer supports operation
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::ShaderResource:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ShaderResource), "ShaderResource flag required on imported buffer, operation invalid");
          break;
        case ezGALResourceState::ConstantBuffer:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer), "ConstantBuffer flag required on imported buffer, operation invalid");
          break;
        case ezGALResourceState::VertexBuffer:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::VertexBuffer), "VertexBuffer flag required on imported buffer, operation invalid");
          break;
        case ezGALResourceState::IndexBuffer:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::IndexBuffer), "IndexBuffer flag required on imported buffer, operation invalid");
          break;
        case ezGALResourceState::DrawIndirect:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::DrawIndirect), "DrawIndirect flag required on imported buffer, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp buffer
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::ShaderResource:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::ShaderResource;
          break;
        case ezGALResourceState::ConstantBuffer:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::ConstantBuffer;
          break;
        case ezGALResourceState::VertexBuffer:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::VertexBuffer;
          break;
        case ezGALResourceState::IndexBuffer:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::IndexBuffer;
          break;
        case ezGALResourceState::DrawIndirect:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::DrawIndirect;
          break;
        default:
          break;
      }
    }
  }

  m_pParent->m_ReadBuffers.PushBack({hBuffer, stage, access});
  m_pParent->m_pCurrentPass->m_uiReadBufferCount++;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::WriteBuffer(ezRenderGraphBufferHandle hBuffer,
  ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG((access & ezGALResourceState::AllBufferStates) == access, "WriteBuffer: Only buffer states are allowed");
  EZ_ASSERT_DEBUG(hBuffer.GetInternalID().m_InstanceIndex < m_pParent->m_BufferCreationDescriptions.GetCount(), "WriteBuffer: hBuffer is invalid");
  EZ_ASSERT_DEBUG(access.IsAnySet(ezGALResourceState::AllWriteStates), "WriteBuffer: Only write states are allowed");
  ezGALBufferCreationDescription& desc = m_pParent->m_BufferCreationDescriptions[hBuffer.GetInternalID().m_InstanceIndex];

  const bool bIsImported = m_pParent->m_HandleToImportBuffer.Contains(hBuffer);
  if (bIsImported)
  {
    EZ_ASSERT_DEBUG(!desc.m_ResourceAccess.m_bImmutable, "Can't write to immutable buffers");
    // Verify imported buffer supports operation
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::UnorderedAccess:
          EZ_ASSERT_DEBUG(desc.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess), "UnorderedAccess flag required on imported buffer, operation invalid");
          break;
        default:
          break;
      }
    }
  }
  else
  {
    // Extend usage flags on temp buffer
    for (ezGALResourceState::Enum flag : access)
    {
      switch (flag)
      {
        case ezGALResourceState::UnorderedAccess:
          desc.m_BufferFlags |= ezGALBufferUsageFlags::UnorderedAccess;
          break;
        default:
          break;
      }
    }
  }

  m_pParent->m_WriteBuffers.PushBack({hBuffer, stage, access});
  m_pParent->m_pCurrentPass->m_uiWriteBufferCount++;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::AddColorTarget(ezRenderGraphTextureHandle hTexture,
  ezGALRenderTargetRange range, ezEnum<ezGALRenderTargetLoadOp> loadOp, ezEnum<ezGALRenderTargetStoreOp> storeOp, ezEnum<ezGALResourceFormat> overrideViewFormat, ezEnum<ezGALTextureType> overrideViewType)
{
  const ezGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  ClampRenderTarget(range, desc);

  ezUInt8& uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
  m_pParent->m_ColorTargets.PushBack({hTexture, range, loadOp, storeOp, overrideViewFormat, overrideViewType});
  ++uiColorTargetCount;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::SetClearColor(ezUInt8 uiIndex, const ezColor& color)
{
  ezUInt8& uiColorTargetCount = m_pParent->m_pCurrentPass->m_uiColorTargetCount;
  ezUInt8& uiClearColorCount = m_pParent->m_pCurrentPass->m_uiClearColorCount;
  EZ_ASSERT_DEBUG(uiIndex < uiColorTargetCount, "Render target not set, call AddColorTarget first");

  while (uiClearColorCount <= uiIndex)
  {
    m_pParent->m_ClearColors.PushBack({});
    uiClearColorCount++;
  }
  m_pParent->m_ClearColors[m_pParent->m_pCurrentPass->m_uiClearColorIndex + uiIndex] = color;
  m_pParent->m_ColorTargets[m_pParent->m_pCurrentPass->m_uiColorTargetIndex + uiIndex].m_loadOp = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::AddDepthStencilTarget(ezRenderGraphTextureHandle hTexture,
  ezGALRenderTargetRange range,
  ezEnum<ezGALRenderTargetLoadOp> depthLoadOp, ezEnum<ezGALRenderTargetStoreOp> depthStoreOp,
  ezEnum<ezGALRenderTargetLoadOp> stencilLoadOp, ezEnum<ezGALRenderTargetStoreOp> stencilStoreOp, bool bReadOnly)
{
  const ezGALTextureCreationDescription& desc = m_pParent->m_TextureCreationDescriptions[hTexture.GetInternalID().m_InstanceIndex];
  ClampRenderTarget(range, desc);

  EZ_ASSERT_DEBUG(m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount == 0, "Multiple depth/stencil targets are not supported");

  m_pParent->m_DepthStencilTargets.PushBack({hTexture, range, depthLoadOp, depthStoreOp, stencilLoadOp, stencilStoreOp, bReadOnly});
  m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount++;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::SetClearDepth(float fDepthClear)
{
  ezUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
  EZ_ASSERT_DEBUG(uiDepthTargetCount == 1, "Render target not set, call AddDepthStencilTarget first");

  if (m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount == 0)
  {
    m_pParent->m_ClearDepthStencils.ExpandAndGetRef().fDepthClear = fDepthClear;
    m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount = 1;
  }
  else
  {
    m_pParent->m_ClearDepthStencils.PeekBack().fDepthClear = fDepthClear;
  }
  m_pParent->m_DepthStencilTargets[m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex].m_depthLoadOp = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::SetClearStencil(ezUInt8 uiStencilClear)
{
  ezUInt8 uiDepthTargetCount = m_pParent->m_pCurrentPass->m_uiDepthStencilTargetCount;
  EZ_ASSERT_DEBUG(uiDepthTargetCount == 1, "Render target not set, call AddDepthStencilTarget first");
  if (m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount == 0)
  {
    m_pParent->m_ClearDepthStencils.ExpandAndGetRef().uiStencilClear = uiStencilClear;
    m_pParent->m_pCurrentPass->m_uiDepthStencilClearCount = 1;
  }
  else
  {
    m_pParent->m_ClearDepthStencils.PeekBack().uiStencilClear = uiStencilClear;
  }
  m_pParent->m_DepthStencilTargets[m_pParent->m_pCurrentPass->m_uiDepthStencilTargetIndex].m_stencilLoadOp = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::HasSideEffects()
{
  m_pParent->m_pCurrentPass->m_bHasSideEffects = true;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::SetStereoscopic(bool bStereoscopic)
{
  m_pParent->m_pCurrentPass->m_bStereoscopic = bStereoscopic;
  return *this;
}

ezRenderGraphPassBuilder& ezRenderGraphPassBuilder::SetExecuteCallback(ezRenderGraphExecuteFunction callback)
{
  m_pParent->m_pCurrentPass->m_ExecuteFunction = callback;
  return *this;
}
