#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

namespace ezStateMachineInternal
{
  /// \brief Helper class to manage instance data for compound states or transitions
  struct EZ_GAMEENGINE_DLL Compound
  {
    EZ_ALWAYS_INLINE ezUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<ezUInt32>(); }
    EZ_ALWAYS_INLINE ezUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    ezSmallArray<ezUInt32, 2> m_InstanceDataOffsets;
    ezInstanceDataAllocator m_InstanceDataAllocator;

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
    bool GetInstanceDataDesc(ezArrayPtr<T*> subObjects, ezInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      ezUInt32 uiMaxAlignment = 0;

      ezInstanceDataDesc instanceDataDesc;
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
