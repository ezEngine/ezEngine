#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraphContext.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphResourceAllocator.h>
#include <RendererCore/RenderGraph/RenderGraphResourcePool.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

// --- Transient Resource Creation ---

ezRenderGraph::ezRenderGraph(ezGALDevice* pDevice, ezStringView sName, ezEnum<ezRenderGraphPhase> phase)
  : m_pDevice(pDevice)
  , m_sGraphName(sName)
  , m_Phase(phase)
{
  m_pAllocator = EZ_DEFAULT_NEW(ezRenderGraphResourceAllocator, ezRenderGraphManager::GetResourcePool());
}

ezRenderGraph::~ezRenderGraph()
{
  Reset();
  ezRenderGraphManager::OnGraphDestroyed(this);
}

ezRenderGraphTextureHandle ezRenderGraph::CreateTexture(const ezGALTextureCreationDescription& desc)
{
  auto id = ezRenderGraphTextureHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_TextureCreationDescriptions.GetCount();

  EZ_ASSERT_DEBUG(desc.Validate(m_pDevice).Succeeded(), "Invalid texture desc");

  m_TextureCreationDescriptions.PushBack(desc);
  return id;
}

ezRenderGraphBufferHandle ezRenderGraph::CreateBuffer(const ezGALBufferCreationDescription& desc)
{
  auto id = ezRenderGraphBufferHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_BufferCreationDescriptions.GetCount();

  m_BufferCreationDescriptions.PushBack(desc);
  m_BufferCreationDescriptions.PeekBack().m_BufferFlags |= ezGALBufferUsageFlags::Transient;
  return id;
}

// --- Resource Queries ---

const ezGALTextureCreationDescription& ezRenderGraph::GetTextureDesc(ezRenderGraphTextureHandle hTexture) const
{
  EZ_ASSERT_DEBUG(hTexture.m_InternalId.m_InstanceIndex < m_TextureCreationDescriptions.GetCount(), "Invalid texture index");
  return m_TextureCreationDescriptions[hTexture.m_InternalId.m_InstanceIndex];
}

const ezGALBufferCreationDescription& ezRenderGraph::GetBufferDesc(ezRenderGraphBufferHandle hBuffer) const
{
  EZ_ASSERT_DEBUG(hBuffer.m_InternalId.m_InstanceIndex < m_BufferCreationDescriptions.GetCount(), "Invalid Buffer index");
  return m_BufferCreationDescriptions[hBuffer.m_InternalId.m_InstanceIndex];
}

// --- Import External Resources ---

ezRenderGraphTextureHandle ezRenderGraph::ImportTexture(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");
  if (ezRenderGraphTextureHandle* hGraphTexture = m_ImportTextureToHandle.GetValue(hTexture))
  {
    // Replace the access state and stage flags on re-import.
    if (ImportedTexture* pImported = m_HandleToImportTexture.GetValue(*hGraphTexture))
    {
      pImported->m_access = access;
      pImported->m_stage = stage;
    }
    return *hGraphTexture;
  }

  auto id = ezRenderGraphTextureHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_TextureCreationDescriptions.GetCount();
  const ezGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  if (pTexture)
  {
    m_TextureCreationDescriptions.PushBack(pTexture->GetDescription());
    m_HandleToImportTexture.Insert(id, ImportedTexture{hTexture, access, stage});
    m_ImportTextureToHandle.Insert(hTexture, id);
  }
  else
  {
    ezLog::Error("Failed to import texture into render graph: invalid handle");
    id.Invalidate();
  }

  return id;
}

ezRenderGraphBufferHandle ezRenderGraph::ImportBuffer(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> access, ezBitflags<ezGALShaderStageFlags> stage)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");
  if (ezRenderGraphBufferHandle* hGraphBuffer = m_ImportBufferToHandle.GetValue(hBuffer))
  {
    // Replace the access state and stage flags on re-import.
    if (ImportedBuffer* pImported = m_HandleToImportBuffer.GetValue(*hGraphBuffer))
    {
      pImported->m_access = access;
      pImported->m_stage = stage;
    }
    return *hGraphBuffer;
  }

  auto id = ezRenderGraphBufferHandle();
  id.m_InternalId.m_Generation = 0;
  id.m_InternalId.m_InstanceIndex = m_BufferCreationDescriptions.GetCount();
  const ezGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer);
  if (pBuffer)
  {
    m_BufferCreationDescriptions.PushBack(pBuffer->GetDescription());
    m_HandleToImportBuffer.Insert(id, ImportedBuffer{hBuffer, access, stage});
    m_ImportBufferToHandle.Insert(hBuffer, id);
  }
  else
  {
    ezLog::Error("Failed to import Buffer into render graph: invalid handle");
    id.Invalidate();
  }

  return id;
}

ezStatus ezRenderGraph::ReplaceImportedTexture(ezRenderGraphTextureHandle hGraphTexture, ezGALTextureHandle hNewTexture)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Operation only allowed between Reset and EnqueueRenderGraph");

  auto it = m_HandleToImportTexture.Find(hGraphTexture);
  // New texture must not be already registered.
  if (m_ImportTextureToHandle.Find(hNewTexture).IsValid())
  {
    if (it.IsValid() && it.Value().m_hTextureHandle != hNewTexture)
      return ezStatus(ezFmt("Replacement texture handle already registered"));
  }
  ezGALTextureCreationDescription& oldDesc = m_TextureCreationDescriptions[hGraphTexture.m_InternalId.m_InstanceIndex];
  const ezGALTexture* pTexture = m_pDevice->GetTexture(hNewTexture);
  if (!pTexture)
    return ezStatus(ezFmt("Replacement texture is invalid"));

  ezGALTextureCreationDescription newDesc = pTexture->GetDescription();
  if (oldDesc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::Presentable))
  {
    if (oldDesc.m_uiWidth != newDesc.m_uiWidth)
      return ezStatus(EZ_FAILURE);
    if (oldDesc.m_uiHeight != newDesc.m_uiHeight)
      return ezStatus(EZ_FAILURE);
  }

  if (oldDesc.m_uiWidth > newDesc.m_uiWidth)
    return ezStatus(ezFmt("Replacement texture width {} is less than the original texture width {}", newDesc.m_uiWidth, oldDesc.m_uiWidth));

  if (oldDesc.m_uiHeight > newDesc.m_uiHeight)
    return ezStatus(ezFmt("Replacement texture height {} is less than the original texture height {}", newDesc.m_uiHeight, oldDesc.m_uiHeight));

  if (oldDesc.m_uiDepth > newDesc.m_uiDepth)
    return ezStatus(ezFmt("Replacement texture depth {} is less than the original texture depth {}", newDesc.m_uiDepth, oldDesc.m_uiDepth));

  if (oldDesc.m_uiArraySize > newDesc.m_uiArraySize)
    return ezStatus(ezFmt("Replacement texture array size {} is less than the original texture array size {}", newDesc.m_uiArraySize, oldDesc.m_uiArraySize));

  if (oldDesc.m_uiMipLevelCount > newDesc.m_uiMipLevelCount)
    return ezStatus(ezFmt("Replacement texture mip levels {} is less than the original texture mip levels {}", newDesc.m_uiMipLevelCount, oldDesc.m_uiMipLevelCount));

  if (oldDesc.m_Format != newDesc.m_Format)
    return ezStatus(ezFmt("Replacement texture format {} is different from original texture format {}", ezArgEnum(newDesc.m_Format), ezArgEnum(oldDesc.m_Format)));

  if (oldDesc.m_SampleCount != newDesc.m_SampleCount)
    return ezStatus(ezFmt("Replacement sample count {} is different from original sample count {}", newDesc.m_SampleCount, oldDesc.m_SampleCount));

  // if (oldDesc.m_Type != newDesc.m_Type)
  //   return ezStatus(ezFmt("Replacement texture type {} is different from original texture type {}", ezArgEnum(newDesc.m_Type), ezArgEnum(oldDesc.m_Type)));

  if (!newDesc.m_TextureFlags.AreAllSet(oldDesc.m_TextureFlags))
    return ezStatus(ezFmt("Replacement texture flags {} does not contain some of the original flags {}", ezArgEnum(newDesc.m_TextureFlags), ezArgEnum(oldDesc.m_TextureFlags)));

  // Is the target graph texture not imported yet?
  if (!it.IsValid())
  {
    oldDesc = newDesc;
    m_HandleToImportTexture.Insert(hGraphTexture, ImportedTexture{hNewTexture, newDesc.GetDefaultState(), ezGALShaderStageFlags::Auto});
    m_ImportTextureToHandle.Insert(hNewTexture, hGraphTexture);
    return EZ_SUCCESS;
  }

  if (it.Value().m_hTextureHandle == hNewTexture)
    return EZ_SUCCESS;

  m_ImportTextureToHandle.Remove(it.Value().m_hTextureHandle);
  it.Value().m_hTextureHandle = hNewTexture;
  m_ImportTextureToHandle[hNewTexture] = hGraphTexture;

  return EZ_SUCCESS;
}

// --- Pass Creation ---

ezRenderGraphPassBuilder ezRenderGraph::AddGraphicsPass(ezStringView sName)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  EZ_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(ezGALQueueType::Graphics, sName);
  return ezRenderGraphPassBuilder(this);
}

ezRenderGraphPassBuilder ezRenderGraph::AddComputePass(ezStringView sName)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  EZ_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(ezGALQueueType::Compute, sName);
  return ezRenderGraphPassBuilder(this);
}

ezRenderGraphPassBuilder ezRenderGraph::AddTransferPass(ezStringView sName)
{
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Recording, "Reset graph first before recording passes again");
  EZ_ASSERT_DEBUG(m_pCurrentPass == nullptr, "An Add*Pass scope is already open");
  StartPassBuilder(ezGALQueueType::Transfer, sName);
  return ezRenderGraphPassBuilder(this);
}

// --- Compilation & Execution ---

void ezRenderGraph::PushMarker(ezStringView sMarker)
{
  EZ_ASSERT_DEV(m_RenderGraphState == RenderGraphState::Recording, "PushMarker can only be called during recording.");
  auto& evt = m_MarkerEvents.ExpandAndGetRef();
  evt.m_uiPassIndex = static_cast<ezUInt16>(m_Passes.GetCount());
  evt.m_bPush = true;
  evt.m_sName = sMarker;
}

void ezRenderGraph::PopMarker()
{
  EZ_ASSERT_DEV(m_RenderGraphState == RenderGraphState::Recording, "PopMarker can only be called during recording.");
  auto& evt = m_MarkerEvents.ExpandAndGetRef();
  evt.m_uiPassIndex = static_cast<ezUInt16>(m_Passes.GetCount());
  evt.m_bPush = false;
}

void ezRenderGraph::Reset()
{
  ResetInternal(RenderGraphState::Recording);
}

ezResult ezRenderGraph::Compile()
{
  EZ_PROFILE_SCOPE("Compile");

  if (m_RenderGraphState < RenderGraphState::Compiled)
  {
    EZ_SUCCEED_OR_RETURN(ValidateGraph());
    BuildDependencyGraph();
    CullDeadPasses();
    BuildSortedPassList();
    ComputeResourceLifetimes();
    AllocateTransientResources();
    BuildRenderingSetups();
    m_RenderGraphState = RenderGraphState::Compiled;
  }
  return EZ_SUCCESS;
}

ezResult ezRenderGraph::GetInspectionInfo(ezRenderGraphInspectionInfo& out_info) const
{
  if (m_RenderGraphState < RenderGraphState::Compiled)
    return EZ_FAILURE;

  // All passes in declaration order, with alive flag.
  const ezUInt32 uiNumPasses = m_Passes.GetCount();
  out_info.m_Passes.SetCount(uiNumPasses);
  for (ezUInt32 i = 0; i < uiNumPasses; ++i)
  {
    auto& pi = out_info.m_Passes[i];
    pi.m_sName = m_PassNames[i];
    pi.m_QueueType = m_Passes[i].m_QueueType;
    pi.m_bHasSideEffects = m_Passes[i].m_bHasSideEffects;
    pi.m_bAlive = m_Alive[i];
  }

  // Textures
  const ezUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  out_info.m_Textures.SetCount(uiNumTextures);
  for (ezUInt32 i = 0; i < uiNumTextures; ++i)
  {
    auto& ti = out_info.m_Textures[i];
    ti.m_Desc = m_TextureCreationDescriptions[i];

    ezRenderGraphTextureHandle hTex;
    hTex.m_InternalId.m_InstanceIndex = i;
    hTex.m_InternalId.m_Generation = 0;
    ti.m_bImported = m_HandleToImportTexture.Contains(hTex);
    ti.m_uiFirstUsePassIndex = m_TextureFirstUse[i];
    ti.m_uiLastUsePassIndex = m_TextureLastUse[i];
    ti.m_uiResolvedIndex = m_TextureToResolvedTexture[i];
  }

  // Buffers
  const ezUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();
  out_info.m_Buffers.SetCount(uiNumBuffers);
  for (ezUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    auto& bi = out_info.m_Buffers[i];
    bi.m_Desc = m_BufferCreationDescriptions[i];

    ezRenderGraphBufferHandle hBuf;
    hBuf.m_InternalId.m_InstanceIndex = i;
    hBuf.m_InternalId.m_Generation = 0;
    bi.m_bImported = m_HandleToImportBuffer.Contains(hBuf);
    bi.m_uiFirstUsePassIndex = m_BufferFirstUse[i];
    bi.m_uiLastUsePassIndex = m_BufferLastUse[i];
    bi.m_uiResolvedIndex = m_BufferToResolvedBuffer[i];
  }

  // Accesses - flatten all pass resource accesses (all passes, not just alive)
  out_info.m_Accesses.Clear();
  out_info.m_Accesses.Reserve(m_ReadBuffers.GetCount() + m_WriteBuffers.GetCount() + m_ReadTextures.GetCount() + m_WriteTextures.GetCount());
  for (ezUInt32 passIdx = 0; passIdx < uiNumPasses; ++passIdx)
  {
    const Pass& pass = m_Passes[passIdx];
    ezUInt16 uiAccessIndexInPass = 0;
    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<ezUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hTexture.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = true;
      a.m_Access = info.m_access;
      a.m_TextureRange = info.m_range;
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<ezUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hTexture.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = true;
      a.m_Access = info.m_access;
      a.m_TextureRange = info.m_range;
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<ezUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = false;
      a.m_Access = info.m_access;
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      auto& a = out_info.m_Accesses.ExpandAndGetRef();
      a.m_uiPassIndex = static_cast<ezUInt16>(passIdx);
      a.m_uiResourceIndex = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      a.m_uiAccessIndex = uiAccessIndexInPass++;
      a.m_bIsTexture = false;
      a.m_Access = info.m_access;
    }
  }

  return EZ_SUCCESS;
}

void ezRenderGraph::Execute(ezRenderGraphContext& ref_ctx, ezArrayPtr<ezRenderGraphPassObserver*> observers)
{
  EZ_ASSERT_DEV(m_RenderGraphState == RenderGraphState::BarriersCreated, "Graph must be compiled before execution");

  ref_ctx.m_pTextureToResolvedTexture = &m_TextureToResolvedTexture;
  ref_ctx.m_pBufferToResolvedBuffer = &m_BufferToResolvedBuffer;
  ref_ctx.m_pResolvedTextures = &m_ResolvedTextures;
  ref_ctx.m_pResolvedBuffers = &m_ResolvedBuffers;
  ref_ctx.m_pUserData = GetUserData();

  // We need to purge bind groups between graphs as otherwise unintended resources get bound that we can't reason about and thus are unable to barrier correctly. E.g. a depth pre-pass might bind the deferred decal atlas from a previous pipeline run.
  ref_ctx.GetRenderContext()->ResetBindGroups();

  {
    ezRenderGraphRenderEvent ev;
    ev.m_Type = ezRenderGraphRenderEvent::Type::BeforeGraphExecution;
    ev.m_pGraph = this;
    ev.m_pContext = &ref_ctx;
    ezRenderGraphManager::s_RenderEvent.Broadcast(ev);
  }

  ezStringBuilder sName = m_sGraphName;
  if (!m_sUserName.IsEmpty())
    sName.AppendFormat("-{}", m_sUserName);

  EZ_PROFILE_AND_MARKER(ref_ctx.GetCommandEncoder(), sName.GetData());

  ezHybridArray<GPUTimingScope*, 4> scopes;
  auto ExecuteMarker = [&](const MarkerEvent& evt, ezGALCommandEncoder* pEncoder)
  {
    if (evt.m_bPush)
    {
      scopes.PushBack(ezProfilingScopeAndMarker::Start(pEncoder, evt.m_sName.GetData()));
    }
    else
    {
      if (!scopes.IsEmpty())
        ezProfilingScopeAndMarker::Stop(pEncoder, scopes.PeekBack());
      scopes.PopBack();
    }
  };

  ezUInt32 uiNextMarker = 0;
  const ezUInt32 uiMarkerCount = m_MarkerEvents.GetCount();
  const ezUInt32 uiPasses = m_CompiledPasses.GetCount();
  for (ezUInt32 i = 0; i < uiPasses; ++i)
  {
    ezRenderGraphManager::s_uiCurrentPassIndex = i;
    const CompiledPass& compiled = m_CompiledPasses[i];
    const Pass& pass = m_Passes[compiled.m_uiOriginalPassIndex];
    const char* szName = m_PassNames[compiled.m_uiOriginalPassIndex].GetData();

    // Emit any markers scheduled before this pass.
    while (uiNextMarker < uiMarkerCount && m_MarkerEvents[uiNextMarker].m_uiPassIndex <= compiled.m_uiOriginalPassIndex)
    {
      ExecuteMarker(m_MarkerEvents[uiNextMarker], ref_ctx.GetCommandEncoder());
      ++uiNextMarker;
    }

    // Issue barriers before the pass.
    auto textureBarriers = compiled.GetTextureBarriers(this);
    if (!textureBarriers.IsEmpty())
    {
      ref_ctx.m_pCommandEncoder->TextureBarrier(textureBarriers);
    }
    auto bufferBarriers = compiled.GetBufferBarriers(this);
    if (!bufferBarriers.IsEmpty())
    {
      ref_ctx.m_pCommandEncoder->BufferBarrier(bufferBarriers);
    }

    ref_ctx.GetRenderContext()->ResetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
    ref_ctx.GetRenderContext()->ResetBindGroup(EZ_GAL_BIND_GROUP_MATERIAL);
    ref_ctx.GetRenderContext()->ResetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);

    // Begin the appropriate scope and invoke the callback.
    switch (pass.m_QueueType)
    {
      case ezGALQueueType::Graphics:
      {
        ezSizeU32 size = compiled.m_RenderingSetup.GetFrameBuffer().m_Size;
        ezRectFloat viewport{0, 0, (float)size.width, (float)size.height};
        if (size.HasNonZeroArea())
        {
          ref_ctx.m_pRenderContext->BeginRendering(compiled.m_RenderingSetup, viewport, szName, pass.m_bStereoscopic);
          if (pass.m_ExecuteFunction.IsValid())
            pass.m_ExecuteFunction(ref_ctx);
          ref_ctx.m_pRenderContext->EndRendering();
        }
        break;
      }
      case ezGALQueueType::Compute:
      {
        ref_ctx.m_pRenderContext->BeginCompute(szName);
        if (pass.m_ExecuteFunction.IsValid())
          pass.m_ExecuteFunction(ref_ctx);
        ref_ctx.m_pRenderContext->EndCompute();
        break;
      }
      case ezGALQueueType::Transfer:
      {
        // Transfer pass - no scope required.
        ref_ctx.m_pCommandEncoder->PushMarker(szName);
        if (pass.m_ExecuteFunction.IsValid())
          pass.m_ExecuteFunction(ref_ctx);
        ref_ctx.m_pCommandEncoder->PopMarker();
        break;
      }
      default:
        EZ_REPORT_FAILURE("Invalid queue type");
        break;
    }

    // Execute observer copies after this pass.
    for (auto* pObserver : observers)
    {
      if (!pObserver->m_bValid || pObserver->m_uiSortedPassIndex != (ezUInt32)i)
        continue;

      if (!pObserver->m_PreCopyBarriers.IsEmpty())
        ref_ctx.m_pCommandEncoder->TextureBarrier(pObserver->m_PreCopyBarriers);

      ref_ctx.m_pCommandEncoder->CopyTexture(pObserver->GetCopyTexture(), pObserver->m_hResolvedSourceTexture);

      if (!pObserver->m_PostCopyBarriers.IsEmpty())
        ref_ctx.m_pCommandEncoder->TextureBarrier(pObserver->m_PostCopyBarriers);
    }
  }

  // Emit any trailing markers (after the last pass).
  while (uiNextMarker < uiMarkerCount)
  {
    ExecuteMarker(m_MarkerEvents[uiNextMarker], ref_ctx.GetCommandEncoder());
    ++uiNextMarker;
  }

  {
    ezRenderGraphRenderEvent ev;
    ev.m_Type = ezRenderGraphRenderEvent::Type::AfterGraphExecution;
    ev.m_pGraph = this;
    ev.m_pContext = &ref_ctx;
    ezRenderGraphManager::s_RenderEvent.Broadcast(ev);
  }
}

void ezRenderGraph::StartPassBuilder(ezEnum<ezGALQueueType> queueType, ezStringView sName)
{
  // Enforce unique pass names by appending a counter suffix on collision.
  ezString sUniqueName = sName;
  ezUInt32& uiSuffix = m_UniquePassNames.FindOrAdd(sUniqueName);
  uiSuffix++;
  if (uiSuffix >= 2)
  {
    ezStringBuilder sTemp;
    sTemp.SetFormat("{} #{}", sName, uiSuffix);
    sUniqueName = sTemp;
  }

  m_PassNames.PushBack(std::move(sUniqueName));
  Pass& pass = m_Passes.ExpandAndGetRef();
  pass.m_QueueType = queueType;
  pass.m_uiReadTextureIndex = m_ReadTextures.GetCount();
  pass.m_uiWriteTextureIndex = m_WriteTextures.GetCount();
  pass.m_uiReadBufferIndex = m_ReadBuffers.GetCount();
  pass.m_uiWriteBufferIndex = m_WriteBuffers.GetCount();
  pass.m_uiColorTargetIndex = m_ColorTargets.GetCount();
  pass.m_uiDepthStencilTargetIndex = m_DepthStencilTargets.GetCount();
  pass.m_uiClearColorIndex = m_ClearColors.GetCount();
  pass.m_uiDepthStencilClearIndex = m_ClearDepthStencils.GetCount();
  m_pCurrentPass = &pass;
}

void ezRenderGraph::EndPassBuilder()
{
  // Just clear the pointer as the Pass was already filled in.
  m_pCurrentPass = nullptr;
}

ezUInt16 ezRenderGraph::FindPassByName(ezStringView sName) const
{
  for (ezUInt32 i = 0; i < m_PassNames.GetCount(); ++i)
  {
    if (m_PassNames[i] == sName)
      return static_cast<ezUInt16>(i);
  }
  return s_Unused;
}

ezRenderGraphTextureHandle ezRenderGraph::FindTextureAccessInPass(ezUInt16 uiOriginalPassIndex, ezUInt16 uiAccessIndex) const
{
  EZ_ASSERT_DEBUG(uiOriginalPassIndex < m_Passes.GetCount(), "Invalid pass index");
  const Pass& pass = m_Passes[uiOriginalPassIndex];

  // Iterate all texture accesses in declaration order: reads, writes, color targets, depth targets.
  ezUInt16 uiCurrent = 0;

  for (const TextureInfo& info : pass.GetReadTextures(this))
  {
    if (uiCurrent == uiAccessIndex)
      return info.m_hTexture;
    ++uiCurrent;
  }
  for (const TextureInfo& info : pass.GetWriteTextures(this))
  {
    if (uiCurrent == uiAccessIndex)
      return info.m_hTexture;
    ++uiCurrent;
  }

  ezRenderGraphTextureHandle invalid;
  invalid.Invalidate();
  return invalid;
}

// --- Private Compile Steps ---

void ezRenderGraph::ResetInternal(RenderGraphState renderGraphState)
{
  if (renderGraphState < RenderGraphState::BarriersCreated)
  {
    // Reset barriers
    m_CompiledTextureBarriers.Clear();
    m_CompiledBufferBarriers.Clear();
    if (renderGraphState == RenderGraphState::Compiled)
    {
      for (CompiledPass& pass : m_CompiledPasses)
      {
        pass.m_uiTextureBarrierIndex = 0;
        pass.m_uiBufferBarrierIndex = 0;
        pass.m_uiTextureBarrierCount = 0;
        pass.m_uiBufferBarrierCount = 0;
      }
    }
  }

  if (renderGraphState < RenderGraphState::Compiled)
  {
    // Reset compile state
    m_CompiledPasses.Clear();

    // Reset intermediate state
    m_AdjacencyStorage.Clear();
    m_Alive.Clear();
    m_AlivePasses.Clear();
    m_TextureFirstUse.Clear();
    m_TextureLastUse.Clear();
    m_BufferFirstUse.Clear();
    m_BufferLastUse.Clear();

    m_AcquireTextures.Clear();
    m_ReleaseTextures.Clear();
    m_AcquireBuffers.Clear();
    m_ReleaseBuffers.Clear();
    m_TextureToResolvedTexture.Clear();
    m_BufferToResolvedBuffer.Clear();

    m_ResolvedTextures.Clear();
    m_ResolvedBuffers.Clear();

    m_pAllocator->FreeResources();
  }

  if (renderGraphState < RenderGraphState::Enqueued)
  {
    // Reset recording state
    m_pCurrentPass = nullptr;

    m_PassNames.Clear();
    m_UniquePassNames.Clear();
    m_Passes.Clear();
    m_ReadTextures.Clear();
    m_WriteTextures.Clear();
    m_ReadBuffers.Clear();
    m_WriteBuffers.Clear();
    m_ColorTargets.Clear();
    m_DepthStencilTargets.Clear();
    m_ClearColors.Clear();
    m_ClearDepthStencils.Clear();
    m_MarkerEvents.Clear();

    m_TextureCreationDescriptions.Clear();
    m_BufferCreationDescriptions.Clear();

    m_HandleToImportTexture.Clear();
    m_ImportTextureToHandle.Clear();
    m_HandleToImportBuffer.Clear();
    m_ImportBufferToHandle.Clear();
  }

  m_RenderGraphState = renderGraphState;
}


ezResult ezRenderGraph::ValidateImportedResources() const
{
  for (auto it = m_HandleToImportTexture.GetIterator(); it.IsValid(); ++it)
  {
    if (m_pDevice->GetTexture(it.Value().m_hTextureHandle) == nullptr)
    {
      ezLog::Error("Render graph '{}': imported texture handle is no longer valid.", m_sUserName);
      return EZ_FAILURE;
    }
  }

  for (auto it = m_HandleToImportBuffer.GetIterator(); it.IsValid(); ++it)
  {
    if (m_pDevice->GetBuffer(it.Value().m_hBufferHandle) == nullptr)
    {
      ezLog::Error("Render graph '{}': imported buffer handle is no longer valid.", m_sUserName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezRenderGraph::ValidateGraph()
{
  EZ_PROFILE_SCOPE("ValidateGraph");
  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    const Pass& pass = m_Passes[i];

    // Validate texture handle indices.
    auto validateTextureHandle = [&](ezRenderGraphTextureHandle h, const char* szContext) -> ezResult
    {
      if (h.IsInvalidated() || h.m_InternalId.m_InstanceIndex >= m_TextureCreationDescriptions.GetCount())
      {
        ezLog::Error("Pass '{}': invalid texture handle in {}.", m_PassNames[i], szContext);
        return EZ_FAILURE;
      }
      return EZ_SUCCESS;
    };

    auto validateBufferHandle = [&](ezRenderGraphBufferHandle h, const char* szContext) -> ezResult
    {
      if (h.IsInvalidated() || h.m_InternalId.m_InstanceIndex >= m_BufferCreationDescriptions.GetCount())
      {
        ezLog::Error("Pass '{}': invalid buffer handle in {}.", m_PassNames[i], szContext);
        return EZ_FAILURE;
      }
      return EZ_SUCCESS;
    };

    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      EZ_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "ReadTexture"));
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      EZ_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "WriteTexture"));
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      EZ_SUCCEED_OR_RETURN(validateBufferHandle(info.m_hBuffer, "ReadBuffer"));
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      EZ_SUCCEED_OR_RETURN(validateBufferHandle(info.m_hBuffer, "WriteBuffer"));
    }
    for (const ColorTargetInfo& info : pass.GetColorTargets(this))
    {
      EZ_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "ColorTarget"));
    }
    for (const DepthStencilTargetInfo& info : pass.GetDepthStencilTargets(this))
    {
      EZ_SUCCEED_OR_RETURN(validateTextureHandle(info.m_hTexture, "DepthStencilTarget"));
    }
  }

  return EZ_SUCCESS;
}

void ezRenderGraph::BuildDependencyGraph()
{
  EZ_PROFILE_SCOPE("BuildDependencyGraph");
  const ezUInt32 uiNumPasses = m_Passes.GetCount();

  // Compute per-pass adjacency capacity and assign offsets into the flat storage.
  // Upper bound per pass: one dependency per resource access.
  ezUInt32 uiTotalCapacity = 0;
  for (ezUInt32 i = 0; i < uiNumPasses; ++i)
  {
    Pass& pass = m_Passes[i];
    pass.m_uiAdjacencyIndex = static_cast<ezUInt16>(uiTotalCapacity);
    pass.m_uiAdjacencyCount = 0;

    const ezUInt32 uiMaxDeps = pass.m_uiReadTextureCount + pass.m_uiReadBufferCount +
                               pass.m_uiWriteTextureCount + pass.m_uiWriteBufferCount;
    uiTotalCapacity += uiMaxDeps;
  }
  m_AdjacencyStorage.SetCount(uiTotalCapacity);

  // Helper: add a dependency edge from pass to writerIdx, deduplicating.
  auto addDependency = [this](Pass& pass, ezUInt16 passIdx, ezUInt16 writerIdx)
  {
    if (writerIdx == passIdx)
      return;

    // Check for duplicates in the current slice.
    const ezUInt16* pBegin = m_AdjacencyStorage.GetData() + pass.m_uiAdjacencyIndex;
    const ezUInt16* pEnd = pBegin + pass.m_uiAdjacencyCount;
    for (const ezUInt16* p = pBegin; p < pEnd; ++p)
    {
      if (*p == writerIdx)
        return;
    }

    m_AdjacencyStorage[pass.m_uiAdjacencyIndex + pass.m_uiAdjacencyCount] = writerIdx;
    ++pass.m_uiAdjacencyCount;
  };

  // We iterate passes in declaration order and track the latest writer per resource.
  ezHashTable<ezUInt32, ezUInt16> textureLastWriter(ezFrameAllocator::GetCurrentAllocator()); // texture instance index -> pass index
  textureLastWriter.Reserve(m_TextureCreationDescriptions.GetCount());
  ezHashTable<ezUInt32, ezUInt16> bufferLastWriter(ezFrameAllocator::GetCurrentAllocator());
  bufferLastWriter.Reserve(m_BufferCreationDescriptions.GetCount());

  for (ezUInt16 passIdx = 0; passIdx < uiNumPasses; ++passIdx)
  {
    Pass& pass = m_Passes[passIdx];

    // Check reads: if this pass reads a resource that was written by an earlier pass, add a dependency.
    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      const ezUInt32 uiTexIdx = info.m_hTexture.m_InternalId.m_InstanceIndex;
      ezUInt16 writerIdx;
      if (textureLastWriter.TryGetValue(uiTexIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      const ezUInt32 uiBufIdx = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      ezUInt16 writerIdx;
      if (bufferLastWriter.TryGetValue(uiBufIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
    }

    // Also check write-after-write: if this pass writes a resource already written by another pass.
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      const ezUInt32 uiTexIdx = info.m_hTexture.m_InternalId.m_InstanceIndex;
      ezUInt16 writerIdx;
      if (textureLastWriter.TryGetValue(uiTexIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
      textureLastWriter[uiTexIdx] = passIdx;
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      const ezUInt32 uiBufIdx = info.m_hBuffer.m_InternalId.m_InstanceIndex;
      ezUInt16 writerIdx;
      if (bufferLastWriter.TryGetValue(uiBufIdx, writerIdx))
      {
        addDependency(pass, passIdx, writerIdx);
      }
      bufferLastWriter[uiBufIdx] = passIdx;
    }

    // Color targets are writes and Depth/stencil targets are already added to the texture read / write arrays during recording.
  }
}

void ezRenderGraph::CullDeadPasses()
{
  EZ_PROFILE_SCOPE("CullDeadPasses");
  const ezUInt32 uiNumPasses = m_Passes.GetCount();

  // Start with all passes marked as dead.
  m_Alive.SetCount(uiNumPasses);
  for (ezUInt32 i = 0; i < uiNumPasses; ++i)
  {
    m_Alive[i] = false;
  }

  // Seed the alive set with passes that have side effects or write to imported resources.
  ezHybridArray<ezUInt16, 16> stack;
  for (ezUInt16 i = 0; i < uiNumPasses; ++i)
  {
    const Pass& pass = m_Passes[i];
    bool bIsRoot = pass.m_bHasSideEffects;

    if (!bIsRoot)
    {
      // Check if this pass writes to an imported texture.
      for (const TextureInfo& info : pass.GetWriteTextures(const_cast<ezRenderGraph*>(this)))
      {
        if (m_HandleToImportTexture.Contains(info.m_hTexture))
        {
          bIsRoot = true;
          break;
        }
      }
    }
    if (!bIsRoot)
    {
      for (const BufferInfo& info : pass.GetWriteBuffers(const_cast<ezRenderGraph*>(this)))
      {
        if (m_HandleToImportBuffer.Contains(info.m_hBuffer))
        {
          bIsRoot = true;
          break;
        }
      }
    }

    if (bIsRoot)
    {
      m_Alive[i] = true;
      stack.PushBack(i);
    }
  }

  // Flood-fill backwards through dependencies.
  while (!stack.IsEmpty())
  {
    const ezUInt16 current = stack.PeekBack();
    stack.PopBack();

    for (ezUInt16 dep : m_Passes[current].GetAdjacency(this))
    {
      if (!m_Alive[dep])
      {
        m_Alive[dep] = true;
        stack.PushBack(dep);
      }
    }
  }
}

void ezRenderGraph::BuildSortedPassList()
{
  EZ_PROFILE_SCOPE("BuildSortedPassList");
  m_AlivePasses.Clear();

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Alive[i])
    {
      m_AlivePasses.ExpandAndGetRef().uiOriginalPassIndex = i;
    }
  }
}

void ezRenderGraph::ComputeResourceLifetimes()
{
  EZ_PROFILE_SCOPE("ComputeResourceLifetimes");
  const ezUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  const ezUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();


  m_TextureFirstUse.SetCount(uiNumTextures, s_Unused);
  m_TextureLastUse.SetCount(uiNumTextures, s_Unused);
  m_BufferFirstUse.SetCount(uiNumBuffers, s_Unused);
  m_BufferLastUse.SetCount(uiNumBuffers, s_Unused);

  auto touchTexture = [&](const ezRenderGraphTextureHandle& hTexture, ezUInt16 uiSortedIndex)
  {
    if (m_HandleToImportTexture.Contains(hTexture))
      return;

    const ezUInt32 uiTexIndex = hTexture.GetInternalID().m_InstanceIndex;
    if (m_TextureFirstUse[uiTexIndex] == s_Unused)
      m_TextureFirstUse[uiTexIndex] = uiSortedIndex;
    m_TextureLastUse[uiTexIndex] = uiSortedIndex;
  };

  auto touchBuffer = [&](const ezRenderGraphBufferHandle& hBuffer, ezUInt16 uiSortedIndex)
  {
    if (m_HandleToImportBuffer.Contains(hBuffer))
      return;

    const ezUInt32 uiBufIndex = hBuffer.GetInternalID().m_InstanceIndex;
    if (m_BufferFirstUse[uiBufIndex] == s_Unused)
      m_BufferFirstUse[uiBufIndex] = uiSortedIndex;
    m_BufferLastUse[uiBufIndex] = uiSortedIndex;
  };

  const ezUInt32 uiSortedPasses = m_AlivePasses.GetCount();
  for (ezUInt32 sortedIdx = 0; sortedIdx < uiSortedPasses; ++sortedIdx)
  {
    const ezUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];

    for (const TextureInfo& info : pass.GetReadTextures(this))
    {
      touchTexture(info.m_hTexture, sortedIdx);
    }
    for (const TextureInfo& info : pass.GetWriteTextures(this))
    {
      touchTexture(info.m_hTexture, sortedIdx);
    }
    for (const BufferInfo& info : pass.GetReadBuffers(this))
    {
      touchBuffer(info.m_hBuffer, sortedIdx);
    }
    for (const BufferInfo& info : pass.GetWriteBuffers(this))
    {
      touchBuffer(info.m_hBuffer, sortedIdx);
    }
  }

  ezUInt32 uiAcquireTextureTotal = 0;
  ezUInt32 uiAcquireBufferTotal = 0;
  for (ezUInt32 i = 0; i < uiNumTextures; ++i)
  {
    const ezUInt16 uiAcquireTexturePass = m_TextureFirstUse[i];
    if (uiAcquireTexturePass != s_Unused)
    {
      uiAcquireTextureTotal++;
      m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount++;
    }
    const ezUInt16 uiReleaseTexturePass = m_TextureLastUse[i];
    if (uiReleaseTexturePass != s_Unused)
    {
      m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount++;
    }
  }
  for (ezUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    const ezUInt16 uiAcquireBufferPass = m_BufferFirstUse[i];
    if (uiAcquireBufferPass != s_Unused)
    {
      uiAcquireBufferTotal++;
      m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount++;
    }
    const ezUInt16 uiReleaseBufferPass = m_BufferLastUse[i];
    if (uiReleaseBufferPass != s_Unused)
    {
      m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount++;
    }
  }

  // Prefix sums to get per-pass acquire / release indices.
  ezUInt32 acquireTextureCounter = 0;
  ezUInt32 releaseTextureCounter = 0;
  ezUInt32 acquireBufferCounter = 0;
  ezUInt32 releaseBufferCounter = 0;
  for (ezUInt32 i = 0; i < uiSortedPasses; ++i)
  {
    m_AlivePasses[i].m_uiAcquireTextureIndex = acquireTextureCounter;
    acquireTextureCounter += m_AlivePasses[i].m_uiAcquireTextureCount;
    m_AlivePasses[i].m_uiAcquireTextureCount = 0;

    m_AlivePasses[i].m_uiReleaseTextureIndex = releaseTextureCounter;
    releaseTextureCounter += m_AlivePasses[i].m_uiReleaseTextureCount;
    m_AlivePasses[i].m_uiReleaseTextureCount = 0;

    m_AlivePasses[i].m_uiAcquireBufferIndex = acquireBufferCounter;
    acquireBufferCounter += m_AlivePasses[i].m_uiAcquireBufferCount;
    m_AlivePasses[i].m_uiAcquireBufferCount = 0;

    m_AlivePasses[i].m_uiReleaseBufferIndex = releaseBufferCounter;
    releaseBufferCounter += m_AlivePasses[i].m_uiReleaseBufferCount;
    m_AlivePasses[i].m_uiReleaseBufferCount = 0;
  }

  // #TODO: Can SetCountUninitialized as we will write to all entries.
  m_AcquireTextures.SetCount(uiAcquireTextureTotal);
  m_ReleaseTextures.SetCount(uiAcquireTextureTotal);
  m_AcquireBuffers.SetCount(uiAcquireBufferTotal);
  m_ReleaseBuffers.SetCount(uiAcquireBufferTotal);

  // We have computed all acquire / release counts and indices. The count was reset as we now scatter write the allocations into the per-pass arrays.
  for (ezUInt32 i = 0; i < uiNumTextures; ++i)
  {
    const ezUInt16 uiAcquireTexturePass = m_TextureFirstUse[i];
    if (uiAcquireTexturePass != s_Unused)
    {
      const ezUInt16 acquireIndex = m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureIndex + m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount;
      m_AcquireTextures[acquireIndex] = i;
      m_AlivePasses[uiAcquireTexturePass].m_uiAcquireTextureCount++;
    }
    const ezUInt16 uiReleaseTexturePass = m_TextureLastUse[i];
    if (uiReleaseTexturePass != s_Unused)
    {
      const ezUInt16 releaseIndex = m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureIndex + m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount;
      m_ReleaseTextures[releaseIndex] = i;
      m_AlivePasses[uiReleaseTexturePass].m_uiReleaseTextureCount++;
    }
  }

  for (ezUInt32 i = 0; i < uiNumBuffers; ++i)
  {
    const ezUInt16 uiAcquireBufferPass = m_BufferFirstUse[i];
    if (uiAcquireBufferPass != s_Unused)
    {
      const ezUInt16 acquireIndex = m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferIndex + m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount;
      m_AcquireBuffers[acquireIndex] = i;
      m_AlivePasses[uiAcquireBufferPass].m_uiAcquireBufferCount++;
    }
    const ezUInt16 uiReleaseBufferPass = m_BufferLastUse[i];
    if (uiReleaseBufferPass != s_Unused)
    {
      const ezUInt16 releaseIndex = m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferIndex + m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount;
      m_ReleaseBuffers[releaseIndex] = i;
      m_AlivePasses[uiReleaseBufferPass].m_uiReleaseBufferCount++;
    }
  }
}

void ezRenderGraph::AllocateTransientResources()
{
  EZ_PROFILE_SCOPE("AllocateTransientResources");

  const ezUInt32 uiNumTextures = m_TextureCreationDescriptions.GetCount();
  const ezUInt32 uiNumBuffers = m_BufferCreationDescriptions.GetCount();

  m_TextureToResolvedTexture.SetCount(uiNumTextures, s_Unused);
  m_BufferToResolvedBuffer.SetCount(uiNumBuffers, s_Unused);
  m_ResolvedTextures.Reserve(uiNumTextures);
  m_ResolvedBuffers.Reserve(uiNumBuffers);

  // Seed resolved mappings with imported resources.
  for (auto it = m_HandleToImportTexture.GetIterator(); it.IsValid(); ++it)
  {
    m_TextureToResolvedTexture[it.Key().m_InternalId.m_InstanceIndex] = m_ResolvedTextures.GetCount();
    m_ResolvedTextures.PushBack(it.Value().m_hTextureHandle);
  }
  for (auto it = m_HandleToImportBuffer.GetIterator(); it.IsValid(); ++it)
  {
    m_BufferToResolvedBuffer[it.Key().m_InternalId.m_InstanceIndex] = m_ResolvedBuffers.GetCount();
    m_ResolvedBuffers.PushBack(it.Value().m_hBufferHandle);
  }

  // Go through all intermediate passes and allocate / release transient resources.
  ezHashTable<ezGALTextureHandle, ezUInt16> TextureToResolvedTextureIndex(ezFrameAllocator::GetCurrentAllocator());
  TextureToResolvedTextureIndex.Reserve(uiNumTextures);
  ezHashTable<ezGALBufferHandle, ezUInt16> BufferToResolvedBufferIndex(ezFrameAllocator::GetCurrentAllocator());
  BufferToResolvedBufferIndex.Reserve(uiNumBuffers);
  // By calling Acquire / Release in chronological order the ezRenderGraphResourceAllocator can alias resources that don't overlap by giving out the same resource multiple times.
  for (ezUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const IntermediatePass& pass = m_AlivePasses[sortedIdx];
    ezArrayPtr<const ezUInt16> acquireTextures = pass.GetAcquireTextures(this);
    for (ezUInt16 uiTextureIndex : acquireTextures)
    {
      ezGALTextureHandle hTexture = m_pAllocator->AcquireTexture(m_TextureCreationDescriptions[uiTextureIndex]);
      bool bExisted = false;
      ezUInt16& uiResolvedTextureIndex = TextureToResolvedTextureIndex.FindOrAdd(hTexture, &bExisted);
      if (!bExisted)
      {
        uiResolvedTextureIndex = m_ResolvedTextures.GetCount();
        m_ResolvedTextures.PushBack(hTexture);
      }
      m_TextureToResolvedTexture[uiTextureIndex] = uiResolvedTextureIndex;
    }

    ezArrayPtr<const ezUInt16> releaseTextures = pass.GetReleaseTextures(this);
    for (ezUInt16 uiTextureIndex : releaseTextures)
    {
      const ezUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[uiTextureIndex];
      m_pAllocator->ReleaseTexture(m_ResolvedTextures[uiResolvedTextureIndex]);
    }

    ezArrayPtr<const ezUInt16> acquireBuffers = pass.GetAcquireBuffers(this);
    for (ezUInt16 uiBufferIndex : acquireBuffers)
    {
      ezGALBufferHandle hBuffer = m_pAllocator->AcquireBuffer(m_BufferCreationDescriptions[uiBufferIndex]);
      bool bExisted = false;
      ezUInt16& uiResolvedBufferIndex = BufferToResolvedBufferIndex.FindOrAdd(hBuffer, &bExisted);
      if (!bExisted)
      {
        uiResolvedBufferIndex = m_ResolvedBuffers.GetCount();
        m_ResolvedBuffers.PushBack(hBuffer);
      }
      m_BufferToResolvedBuffer[uiBufferIndex] = uiResolvedBufferIndex;
    }

    ezArrayPtr<const ezUInt16> releaseBuffers = pass.GetReleaseBuffers(this);
    for (ezUInt16 uiBufferIndex : releaseBuffers)
    {
      const ezUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[uiBufferIndex];
      m_pAllocator->ReleaseBuffer(m_ResolvedBuffers[uiResolvedBufferIndex]);
    }
  }
}

void ezRenderGraph::BuildRenderingSetups()
{
  EZ_PROFILE_SCOPE("BuildRenderingSetups");
  m_CompiledPasses.SetCount(m_AlivePasses.GetCount());

  for (ezUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const ezUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];
    CompiledPass& compiled = m_CompiledPasses[sortedIdx];
    compiled.m_uiOriginalPassIndex = passIdx;

    const auto colorTargets = pass.GetColorTargets(this);
    const auto depthTargets = pass.GetDepthStencilTargets(this);

    if (colorTargets.IsEmpty() && depthTargets.IsEmpty())
      continue;

    ezGALRenderingSetup& setup = compiled.m_RenderingSetup;

    // Color targets.
    for (ezUInt8 i = 0; i < colorTargets.GetCount(); ++i)
    {
      const ColorTargetInfo& ct = colorTargets[i];
      const ezUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[ct.m_hTexture.m_InternalId.m_InstanceIndex];
      const ezGALTextureHandle hGAL = m_ResolvedTextures[uiResolvedTextureIndex];
      auto pTexture = m_pDevice->GetTexture(hGAL);

      ezGALRenderTargetViewCreationDescription rtvDesc;
      rtvDesc.m_hTexture = hGAL;
      rtvDesc.m_uiMipLevel = ct.m_range.m_uiBaseMipLevel;
      rtvDesc.m_uiFirstSlice = ct.m_range.m_uiBaseArraySlice;
      rtvDesc.m_uiSliceCount = ct.m_range.m_uiArraySlices;
      rtvDesc.m_OverrideViewFormat = ct.m_overrideViewFormat;
      rtvDesc.m_OverrideViewType = ct.m_overrideViewType;

      ezGALRenderTargetViewHandle hRTV = m_pDevice->GetRenderTargetView(rtvDesc);
      if (pTexture->GetDescription().m_Type == ezGALTextureType::Texture2DProxy)
      {
        hRTV = m_pDevice->GetDefaultRenderTargetView(hGAL);
      }


      setup.SetColorTarget(i, hRTV, ct.m_loadOp, ct.m_storeOp);
    }

    // Apply clear colors.
    const auto clearColors = pass.GetClearColors(this);
    for (ezUInt8 i = 0; i < clearColors.GetCount(); ++i)
    {
      setup.SetClearColor(i, clearColors[i]);
    }

    // Depth/stencil target (at most one).
    if (!depthTargets.IsEmpty())
    {
      const DepthStencilTargetInfo& ds = depthTargets[0];
      const ezUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[ds.m_hTexture.m_InternalId.m_InstanceIndex];
      const ezGALTextureHandle hGAL = m_ResolvedTextures[uiResolvedTextureIndex];

      ezGALRenderTargetViewCreationDescription rtvDesc;
      rtvDesc.m_hTexture = hGAL;
      rtvDesc.m_uiMipLevel = ds.m_range.m_uiBaseMipLevel;
      rtvDesc.m_uiFirstSlice = ds.m_range.m_uiBaseArraySlice;
      rtvDesc.m_uiSliceCount = ds.m_range.m_uiArraySlices;
      rtvDesc.m_bReadOnly = ds.m_bReadOnly;

      ezGALRenderTargetViewHandle hDSV = m_pDevice->GetRenderTargetView(rtvDesc);
      setup.SetDepthStencilTarget(hDSV, ds.m_depthLoadOp, ds.m_depthStoreOp, ds.m_stencilLoadOp, ds.m_stencilStoreOp);

      // Apply clear depth/stencil.
      const auto clearDS = pass.GetClearDepthStencils(this);
      if (!clearDS.IsEmpty())
      {
        setup.SetClearDepth(clearDS[0].fDepthClear);
        setup.SetClearStencil(clearDS[0].uiStencilClear);
      }
    }
  }
}

void ezRenderGraph::ComputeBarriers(ezGALResourceStateTracker& ref_tracker, ezArrayPtr<ezRenderGraphPassObserver*> observers)
{
  EZ_PROFILE_SCOPE("ComputeBarriers");
  EZ_ASSERT_DEBUG(m_RenderGraphState == RenderGraphState::Compiled, "ComputeBarriers must be called after Compile succeeded");

  for (ezUInt32 sortedIdx = 0; sortedIdx < m_AlivePasses.GetCount(); ++sortedIdx)
  {
    const ezUInt16 passIdx = m_AlivePasses[sortedIdx].uiOriginalPassIndex;
    const Pass& pass = m_Passes[passIdx];
    CompiledPass& compiled = m_CompiledPasses[sortedIdx];

    compiled.m_uiTextureBarrierIndex = m_CompiledTextureBarriers.GetCount();
    compiled.m_uiBufferBarrierIndex = m_CompiledBufferBarriers.GetCount();
    if (sortedIdx == 0)
    {
      // Add barriers for imported resources that have explicitly set a resource state.
      for (auto it : m_HandleToImportTexture)
      {
        if (it.Value().m_access != ezGALResourceState::Unknown)
        {
          ref_tracker.ChangeState(it.Value().m_hTextureHandle, {}, it.Value().m_access, it.Value().m_stage, [&](const ezGALTextureBarrier& barrier)
            {
              m_CompiledTextureBarriers.PushBack(barrier); //
            });
        }
      }
      for (auto it : m_HandleToImportBuffer)
      {
        if (it.Value().m_access != ezGALResourceState::Unknown)
        {
          ref_tracker.ChangeState(it.Value().m_hBufferHandle, it.Value().m_access, it.Value().m_stage, [&](const ezGALBufferBarrier& barrier)
            {
              m_CompiledBufferBarriers.PushBack(barrier); //
            });
        }
      }
    }

    auto readTextures = pass.GetReadTextures(this);
    for (const TextureInfo& info : readTextures)
    {
      const ezUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
      ezGALTextureHandle hTexture = m_ResolvedTextures[uiResolvedTextureIndex];
      ref_tracker.ChangeState(hTexture, info.m_range, info.m_access, info.m_stage, [&](const ezGALTextureBarrier& barrier)
        {
          m_CompiledTextureBarriers.PushBack(barrier); //
        });
    }
    auto writeTextures = pass.GetWriteTextures(this);
    for (const TextureInfo& info : writeTextures)
    {
      const ezUInt16 uiResolvedTextureIndex = m_TextureToResolvedTexture[info.m_hTexture.m_InternalId.m_InstanceIndex];
      ezGALTextureHandle hTexture = m_ResolvedTextures[uiResolvedTextureIndex];
      ref_tracker.ChangeState(hTexture, info.m_range, info.m_access, info.m_stage, [&](const ezGALTextureBarrier& barrier)
        {
          m_CompiledTextureBarriers.PushBack(barrier); //
        });
    }
    compiled.m_uiTextureBarrierCount = m_CompiledTextureBarriers.GetCount() - compiled.m_uiTextureBarrierIndex;

    auto readBuffers = pass.GetReadBuffers(this);
    for (const BufferInfo& info : readBuffers)
    {
      const ezUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
      ezGALBufferHandle hBuffer = m_ResolvedBuffers[uiResolvedBufferIndex];
      ref_tracker.ChangeState(hBuffer, info.m_access, info.m_stage, [&](const ezGALBufferBarrier& barrier)
        {
          m_CompiledBufferBarriers.PushBack(barrier); //
        });
    }
    auto writeBuffers = pass.GetWriteBuffers(this);
    for (const BufferInfo& info : writeBuffers)
    {
      const ezUInt16 uiResolvedBufferIndex = m_BufferToResolvedBuffer[info.m_hBuffer.m_InternalId.m_InstanceIndex];
      ezGALBufferHandle hBuffer = m_ResolvedBuffers[uiResolvedBufferIndex];
      ref_tracker.ChangeState(hBuffer, info.m_access, info.m_stage, [&](const ezGALBufferBarrier& barrier)
        {
          m_CompiledBufferBarriers.PushBack(barrier); //
        });
    }
    compiled.m_uiBufferBarrierCount = m_CompiledBufferBarriers.GetCount() - compiled.m_uiBufferBarrierIndex;

    // Set up observers that target this pass.
    for (auto* pObserver : observers)
    {
      if (pObserver->GetRequest().m_sPassName != m_PassNames[passIdx])
        continue;

      const ezRenderGraphTextureHandle hSourceTex = FindTextureAccessInPass(passIdx, pObserver->GetRequest().m_uiAccessIndex);
      if (hSourceTex.IsInvalidated())
        continue;

      const ezUInt16 uiResolvedIdx = m_TextureToResolvedTexture[hSourceTex.m_InternalId.m_InstanceIndex];
      if (uiResolvedIdx == s_Unused)
        continue;

      const ezGALTextureHandle hResolvedSource = m_ResolvedTextures[uiResolvedIdx];
      const ezGALTextureCreationDescription& srcDesc = m_TextureCreationDescriptions[hSourceTex.m_InternalId.m_InstanceIndex];

      pObserver->EnsureCopyTexture(srcDesc);
      if (pObserver->GetCopyTexture().IsInvalidated())
        continue;

      pObserver->m_hResolvedSourceTexture = hResolvedSource;
      pObserver->m_uiSortedPassIndex = sortedIdx;
      pObserver->m_bValid = true;

      // Source → CopySource barrier (tracked by the state tracker so the
      // next pass's barriers will restore the correct state).
      ref_tracker.ChangeState(hResolvedSource, {}, ezGALResourceState::CopySource, ezGALShaderStageFlags::Auto, [&](const ezGALTextureBarrier& barrier)
        { pObserver->m_PreCopyBarriers.PushBack(barrier); });

      const ezGALResourceState::Enum destReadState = ezGALResourceFormat::IsDepthFormat(srcDesc.m_Format)
                                                       ? ezGALResourceState::DepthStencilRead
                                                       : ezGALResourceState::ShaderResource;

      // Destination → CopyDestination barrier (not tracked — external resource).
      {
        ezGALTextureBarrier destBarrier;
        destBarrier.m_hTexture = pObserver->GetCopyTexture();
        destBarrier.m_StateBefore = destReadState;
        destBarrier.m_StateAfter = ezGALResourceState::CopyDestination;
        pObserver->m_PreCopyBarriers.PushBack(destBarrier);
      }

      // Destination → read barrier after copy.
      {
        ezGALTextureBarrier destBarrier;
        destBarrier.m_hTexture = pObserver->GetCopyTexture();
        destBarrier.m_StateBefore = ezGALResourceState::CopyDestination;
        destBarrier.m_StateAfter = destReadState;
        pObserver->m_PostCopyBarriers.PushBack(destBarrier);
      }
    }
  }
  m_RenderGraphState = RenderGraphState::BarriersCreated;
}
