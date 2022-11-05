#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/StateMachine/Implementation/StateMachineInstanceData.h>

ezUInt32 ezStateMachineInstanceDataAllocator::AddDesc(const ezStateMachineInstanceDataDesc& desc)
{
  m_Descs.PushBack(desc);

  const ezUInt32 uiOffset = ezMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
  m_uiTotalDataSize = uiOffset + desc.m_uiTypeSize;

  return uiOffset;
}

void ezStateMachineInstanceDataAllocator::ClearDescs()
{
  m_Descs.Clear();
  m_uiTotalDataSize = 0;
}

ezBlob ezStateMachineInstanceDataAllocator::AllocateAndConstruct() const
{
  ezBlob blob;
  if (m_uiTotalDataSize > 0)
  {
    blob.SetCountUninitialized(m_uiTotalDataSize);
    blob.ZeroFill();

    Construct(blob);
  }

  return blob;
}

void ezStateMachineInstanceDataAllocator::DestructAndDeallocate(ezBlob& blob) const
{
  EZ_ASSERT_DEV(blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");
  Destruct(blob);

  blob.Clear();
}

void ezStateMachineInstanceDataAllocator::Construct(ezBlob& blob) const
{
  ezUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = ezMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_ConstructorFunction != nullptr)
    {
      desc.m_ConstructorFunction(GetInstanceData(blob, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}

void ezStateMachineInstanceDataAllocator::Destruct(ezBlob& blob) const
{
  ezUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = ezMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_DestructorFunction != nullptr)
    {
      desc.m_DestructorFunction(GetInstanceData(blob, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}
