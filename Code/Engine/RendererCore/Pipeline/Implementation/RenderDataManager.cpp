#include <RendererCore/RendererCorePCH.h>

#include <Core/World/World.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

constexpr ezUInt32 s_uiSkinningBufferIndex = 2;

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezRenderDataManager);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderDataManager, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRenderDataManager::ezRenderDataManager(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
  ezRenderWorld::GetExtractionEvent().AddEventHandler(ezMakeDelegate(&ezRenderDataManager::OnExtractionEvent, this));

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerInstanceData);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Static Instance Data"));
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Dynamic Instance Data"));

  // Skinning buffer
  desc.m_uiStructSize = sizeof(ezShaderTransform);
  desc.m_uiTotalSize = 1024 * desc.m_uiStructSize; // TODO: make initial size configurable

  EZ_ASSERT_DEBUG(m_Buffers.GetCount() == s_uiSkinningBufferIndex, "Unexpected buffer index");
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, "Skinning Data"));
}

ezRenderDataManager::~ezRenderDataManager()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezMakeDelegate(&ezRenderDataManager::OnExtractionEvent, this));

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  for (auto& hBuffer : m_Buffers)
  {
    pDevice->DestroyDynamicBuffer(hBuffer);
  }
}

void ezRenderDataManager::Initialize()
{
  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRenderDataManager::CompactSkinningDataBuffer, this);
    desc.m_Phase = ezWorldUpdatePhase::PostTransform;
    desc.m_fPriority = -1000.0f;

    RegisterUpdateFunction(desc);
  }
}

ezArrayPtr<ezPerInstanceData> ezRenderDataManager::GetOrCreateInstanceData(const ezComponent* pOwnerComponent, bool bDynamic, ezGALDynamicBufferHandle& out_hBuffer, ezInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount /*= 1*/) const
{
  EZ_LOCK(m_Mutex);

  const ezUInt32 uiBufferIndex = bDynamic ? 1 : 0;

  if (inout_instanceDataOffset.IsInvalidated() == false && inout_instanceDataOffset.m_uiIsDynamic != uiBufferIndex)
  {
    // The instance data was allocated in a different buffer, need to re-allocate.
    auto pOldInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[inout_instanceDataOffset.m_uiIsDynamic]);
    pOldInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }

  out_hBuffer = m_Buffers[uiBufferIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiBufferIndex ? m_ExtractionData.m_pBuffers[uiBufferIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  if (inout_instanceDataOffset.IsInvalidated())
  {
    ezComponentHandle hOwnerComponent = pOwnerComponent != nullptr ? pOwnerComponent->GetHandle() : ezComponentHandle();
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(hOwnerComponent, uiCount, ezGALDynamicBuffer::AllocateFlags::None, ezFrameAllocator::GetCurrentAllocator());
    inout_instanceDataOffset.m_uiIsDynamic = uiBufferIndex;
  }

  return pInstanceDataBuffer->MapForWriting<ezPerInstanceData>(inout_instanceDataOffset.m_uiOffset);
}

void ezRenderDataManager::DeleteInstanceData(ezInstanceDataOffset& inout_instanceDataOffset) const
{
  EZ_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    const ezUInt32 uiBufferIndex = inout_instanceDataOffset.m_uiIsDynamic;

    auto pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiBufferIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

ezUInt32 ezRenderDataManager::RegisterCustomInstanceData(const ezGALBufferCreationDescription& desc, ezStringView sDebugName, ezDelegate<void()> beforeUploadCallback /*= {}*/)
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
  {
    auto pBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[i]);
    if (pBuffer->GetDescription() == desc && pBuffer->GetDebugName() == sDebugName)
    {
      return i;
    }
  }

  ezUInt32 uiBufferIndex = m_Buffers.GetCount();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_Buffers.PushBack(pDevice->CreateDynamicBuffer(desc, sDebugName));

  if (beforeUploadCallback.IsValid())
  {
    m_BeforeUploadCallbacks.EnsureCount(uiBufferIndex + 1);
    m_BeforeUploadCallbacks[uiBufferIndex] = beforeUploadCallback;
  }

  return uiBufferIndex;
}

ezByteArrayPtr ezRenderDataManager::GetOrCreateCustomInstanceData(ezUInt32 uiCustomDataIndex, ezUInt32 uiStructByteSize, const ezComponent* pOwnerComponent, ezGALDynamicBufferHandle& out_hBuffer, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiCount) const
{
  EZ_LOCK(m_Mutex);

  out_hBuffer = m_Buffers[uiCustomDataIndex];

  auto pInstanceDataBuffer = m_ExtractionData.m_pBuffers.GetCount() > uiCustomDataIndex ? m_ExtractionData.m_pBuffers[uiCustomDataIndex] : nullptr;
  if (pInstanceDataBuffer == nullptr)
  {
    pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(out_hBuffer);
  }

  EZ_ASSERT_DEV(pInstanceDataBuffer->GetDescription().m_uiStructSize == uiStructByteSize, "Requested struct size {} does not match the registered size {}.", uiStructByteSize, pInstanceDataBuffer->GetDescription().m_uiStructSize);

  if (inout_instanceDataOffset.IsInvalidated())
  {
    inout_instanceDataOffset.m_uiOffset = pInstanceDataBuffer->Allocate(pOwnerComponent->GetHandle(), uiCount, ezGALDynamicBuffer::AllocateFlags::None, ezFrameAllocator::GetCurrentAllocator());
  }

  return pInstanceDataBuffer->MapBytesForWriting(inout_instanceDataOffset.m_uiOffset);
}

void ezRenderDataManager::DeleteCustomInstanceData(ezUInt32 uiCustomDataIndex, ezCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  EZ_LOCK(m_Mutex);

  if (inout_instanceDataOffset.IsInvalidated() == false)
  {
    auto pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

    pInstanceDataBuffer->Deallocate(inout_instanceDataOffset.m_uiOffset);
    inout_instanceDataOffset = {};
  }
}

void ezRenderDataManager::CompactCustomInstanceDataBuffer(ezUInt32 uiCustomDataIndex, ezUInt32 uiMaxSteps)
{
  EZ_LOCK(m_Mutex);

  auto pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[uiCustomDataIndex]);

  ezHybridArray<ezGALDynamicBuffer::ChangedAllocation, 16> changedAllocations;
  pInstanceDataBuffer->RunCompactionSteps(changedAllocations, uiMaxSteps);

  for (const auto& changedAllocation : changedAllocations)
  {
    ezComponentHandle hComponent(ezComponentId(changedAllocation.m_uiUserData));
    ezComponent* pComponent = nullptr;
    EZ_VERIFY(GetWorld()->TryGetComponent(hComponent, pComponent), "Invalid component handle");

    ezMsgCustomInstanceDataOffsetChanged msg;
    msg.m_NewOffset.m_uiOffset = changedAllocation.m_uiNewOffset;
    EZ_VERIFY(pComponent->SendMessage(msg), "Component of type '{}' did not handle ezMsgCustomInstanceDataOffsetChanged.", pComponent->GetDynamicRTTI()->GetTypeName());
  }
}

ezArrayPtr<ezShaderTransform> ezRenderDataManager::GetOrCreateSkinningData(const ezComponent* pOwnerComponent, ezCustomInstanceDataOffset& inout_instanceDataOffset, ezUInt32 uiNumTransforms) const
{
  ezGALDynamicBufferHandle hDummy;
  return GetOrCreateCustomInstanceData<ezShaderTransform>(s_uiSkinningBufferIndex, pOwnerComponent, hDummy, inout_instanceDataOffset, uiNumTransforms);
}

ezArrayPtr<const ezShaderTransform> ezRenderDataManager::GetSkinningData(const ezCustomInstanceDataOffset& instanceDataOffset) const
{
  EZ_LOCK(m_Mutex);

  auto pInstanceDataBuffer = ezGALDevice::GetDefaultDevice()->GetDynamicBuffer(m_Buffers[s_uiSkinningBufferIndex]);

  return pInstanceDataBuffer->MapForReading<ezShaderTransform>(instanceDataOffset.m_uiOffset);
}

void ezRenderDataManager::DeleteSkinningData(ezCustomInstanceDataOffset& inout_instanceDataOffset) const
{
  DeleteCustomInstanceData(s_uiSkinningBufferIndex, inout_instanceDataOffset);
}

ezGALDynamicBufferHandle ezRenderDataManager::GetSkinningDataBuffer() const
{
  return GetCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void ezRenderDataManager::CompactSkinningDataBuffer(const UpdateContext& context)
{
  CompactCustomInstanceDataBuffer(s_uiSkinningBufferIndex);
}

void ezRenderDataManager::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (e.m_Type == ezRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    m_ExtractionData.m_pBuffers.SetCount(m_Buffers.GetCount());

    for (ezUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      m_ExtractionData.m_pBuffers[i] = pDevice->GetDynamicBuffer(m_Buffers[i]);
    }
  }
  else if (e.m_Type == ezRenderWorldExtractionEvent::Type::EndExtraction)
  {
    for (ezUInt32 i = 0; i < m_Buffers.GetCount(); ++i)
    {
      if (m_BeforeUploadCallbacks.GetCount() > i && m_BeforeUploadCallbacks[i].IsValid())
      {
        m_BeforeUploadCallbacks[i]();
      }

      m_ExtractionData.m_pBuffers[i]->UploadChangesForNextFrame();
    }

    m_ExtractionData.m_pBuffers.Clear();
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderDataManager);
