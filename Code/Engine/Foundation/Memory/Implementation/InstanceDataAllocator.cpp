#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/InstanceDataAllocator.h>

ezUInt32 ezInstanceDataAllocator::AddDesc(const ezInstanceDataDesc& desc)
{
  m_Descs.PushBack(desc);

  const ezUInt32 uiOffset = ezMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
  m_uiTotalDataSize = uiOffset + desc.m_uiTypeSize;

  return uiOffset;
}

void ezInstanceDataAllocator::ClearDescs()
{
  m_Descs.Clear();
  m_uiTotalDataSize = 0;
}

ezBlob ezInstanceDataAllocator::AllocateAndConstruct() const
{
  ezBlob blob;
  if (m_uiTotalDataSize > 0)
  {
    blob.SetCountUninitialized(m_uiTotalDataSize);
    blob.ZeroFill();

    Construct(blob.GetByteBlobPtr());
  }

  return blob;
}

void ezInstanceDataAllocator::DestructAndDeallocate(ezBlob& ref_blob) const
{
  EZ_ASSERT_DEV(ref_blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");
  Destruct(ref_blob.GetByteBlobPtr());

  ref_blob.Clear();
}

void ezInstanceDataAllocator::Construct(ezByteBlobPtr blobPtr) const
{
  ezUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = ezMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_ConstructorFunction != nullptr)
    {
      desc.m_ConstructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}

void ezInstanceDataAllocator::Destruct(ezByteBlobPtr blobPtr) const
{
  ezUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = ezMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_DestructorFunction != nullptr)
    {
      desc.m_DestructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}


