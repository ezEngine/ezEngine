#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>

/// \brief Structure to describe an instance data type for a state or transition.
///
/// Since state machine states and transitions are shared between instances they can't hold their state in member
/// variables. This structure describes the type of instance data necessary for a state or transition.
/// Instance data is then automatically allocated by the state machine instance and passed via the pInstanceData pointer
/// in the state or transition functions.
/// Use the templated Fill() method to fill the desc from an instance data type.
struct ezStateMachineInstanceDataDesc
{
  ezUInt32 m_uiTypeSize = 0;
  ezUInt32 m_uiTypeAlignment = 0;
  ezMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  ezMemoryUtils::DestructorFunction m_DestructorFunction = nullptr;

  template <typename T>
  EZ_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize = sizeof(T);
    m_uiTypeAlignment = EZ_ALIGNMENT_OF(T);
    m_ConstructorFunction = ezMemoryUtils::MakeConstructorFunction<T>();
    m_DestructorFunction = ezMemoryUtils::MakeDestructorFunction<T>();
  }
};

namespace ezStateMachineInternal
{
  /// \brief Helper class to manager instance data allocation, construction and destruction
  class EZ_GAMEENGINE_DLL InstanceDataAllocator
  {
  public:
    /// \brief Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
    ezUInt32 AddDesc(const ezStateMachineInstanceDataDesc& desc);

    void ClearDescs();

    void Construct(const ezByteBlobPtr& blobPtr) const;
    void Destruct(const ezByteBlobPtr& blobPtr) const;

    ezBlob AllocateAndConstruct() const;
    void DestructAndDeallocate(ezBlob& ref_blob) const;

    ezUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

    EZ_ALWAYS_INLINE static void* GetInstanceData(const ezByteBlobPtr& blobPtr, ezUInt32 uiOffset)
    {
      return (uiOffset != ezInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
    }

  private:
    ezDynamicArray<ezStateMachineInstanceDataDesc> m_Descs;
    ezUInt32 m_uiTotalDataSize = 0;
  };

  /// \brief Helper class to manage instance data for compound states or transitions
  struct EZ_GAMEENGINE_DLL Compound
  {
    EZ_ALWAYS_INLINE ezUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<ezUInt32>(); }
    EZ_ALWAYS_INLINE ezUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    ezSmallArray<ezUInt32, 2> m_InstanceDataOffsets;
    InstanceDataAllocator m_InstanceDataAllocator;

    struct InstanceData
    {
      const Compound* m_pOwner = nullptr;

      ~InstanceData()
      {
        if (m_pOwner != nullptr)
        {
          m_pOwner->m_InstanceDataAllocator.Destruct(GetBlobPtr());
        }
      }

      EZ_ALWAYS_INLINE ezByteBlobPtr GetBlobPtr()
      {
        return ezByteBlobPtr(ezMemoryUtils::AddByteOffset(reinterpret_cast<ezUInt8*>(this), m_pOwner->GetBaseOffset()), m_pOwner->GetDataSize());
      }
    };

    EZ_ALWAYS_INLINE void* GetSubInstanceData(InstanceData* pData, ezUInt32 uiIndex) const
    {
      return pData != nullptr ? m_InstanceDataAllocator.GetInstanceData(pData->GetBlobPtr(), m_InstanceDataOffsets[uiIndex]) : nullptr;
    }

    EZ_FORCE_INLINE void Initialize(InstanceData* pData) const
    {
      if (pData != nullptr && pData->m_pOwner == nullptr)
      {
        pData->m_pOwner = this;
        m_InstanceDataAllocator.Construct(pData->GetBlobPtr());
      }
    }

    template <typename T>
    bool GetInstanceDataDesc(ezArrayPtr<T*> subObjects, ezStateMachineInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      ezUInt32 uiMaxAlignment = 0;

      ezStateMachineInstanceDataDesc instanceDataDesc;
      for (T* pSubObject : subObjects)
      {
        ezUInt32 uiOffset = ezInvalidIndex;
        if (pSubObject->GetInstanceDataDesc(instanceDataDesc))
        {
          uiOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
          uiMaxAlignment = ezMath::Max(uiMaxAlignment, instanceDataDesc.m_uiTypeAlignment);
        }
        m_InstanceDataOffsets.PushBack(uiOffset);
      }

      if (uiMaxAlignment > 0)
      {
        out_desc.FillFromType<InstanceData>();
        out_desc.m_ConstructorFunction = nullptr; // not needed, instance data is constructed on first OnEnter

        ezUInt32 uiBaseOffset = ezMemoryUtils::AlignSize(out_desc.m_uiTypeSize, uiMaxAlignment);
        m_InstanceDataOffsets.GetUserData<ezUInt32>() = uiBaseOffset;

        out_desc.m_uiTypeSize = uiBaseOffset + m_InstanceDataAllocator.GetTotalDataSize();
        out_desc.m_uiTypeAlignment = ezMath::Max(out_desc.m_uiTypeAlignment, uiMaxAlignment);

        return true;
      }

      return false;
    }
  };
} // namespace ezStateMachineInternal
