#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>

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

class EZ_GAMEENGINE_DLL ezStateMachineInstanceDataAllocator
{
public:
  /// \brief Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
  ezUInt32 AddDesc(const ezStateMachineInstanceDataDesc& desc);

  void ClearDescs();

  void Construct(ezBlob& blob) const;
  void Destruct(ezBlob& blob) const;

  ezBlob AllocateAndConstruct() const;
  void DestructAndDeallocate(ezBlob& blob) const;

  ezUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

  EZ_ALWAYS_INLINE static void* GetInstanceData(ezBlob& blob, ezUInt32 uiOffset)
  {
    return (uiOffset != ezInvalidIndex) ? blob.GetByteBlobPtr().GetPtr() + uiOffset : nullptr;
  }

private:
  ezDynamicArray<ezStateMachineInstanceDataDesc> m_Descs;
  ezUInt32 m_uiTotalDataSize = 0;
};
