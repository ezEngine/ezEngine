#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>

/// \brief Structure to describe an instance data type.
///
/// Many resources, such as VMs, state machines and visual scripts of various types have shared state (their configuration)
/// as well as per-instance state (for their execution).
///
/// This structure describes the type of instance data used by a such a resource (or a node inside it).
/// Instance data is allocated through the ezInstanceDataAllocator.
///
/// Use the templated Fill() method to fill the desc from a data type.
struct EZ_FOUNDATION_DLL ezInstanceDataDesc
{
  ezUInt32 m_uiTypeSize = 0;
  ezUInt32 m_uiTypeAlignment = 0;
  ezMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  ezMemoryUtils::DestructorFunction m_DestructorFunction = nullptr;

  template <typename T>
  EZ_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize = sizeof(T);
    m_uiTypeAlignment = alignof(T);
    m_ConstructorFunction = ezMemoryUtils::MakeConstructorFunction<SkipTrivialTypes, T>();
    m_DestructorFunction = ezMemoryUtils::MakeDestructorFunction<T>();
  }
};

/// \brief Manages complex multi-type instance data allocation with proper construction and destruction.
///
/// This allocator is designed for systems that need to allocate heterogeneous data structures
/// in a single memory block, such as VM instances, state machines, or script execution contexts.
/// It calculates proper alignments for mixed data types and handles construction/destruction
/// automatically.
///
/// Typical workflow:
/// 1. Use AddDesc() to register all data types needed for an instance
/// 2. Call AllocateAndConstruct() to create initialized memory
/// 3. Use GetInstanceData() to access individual data by offset
/// 4. Call DestructAndDeallocate() when the instance is no longer needed
class EZ_FOUNDATION_DLL ezInstanceDataAllocator
{
public:
  /// \brief Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
  [[nodiscard]] ezUInt32 AddDesc(const ezInstanceDataDesc& desc);

  /// \brief Resets all internal state.
  void ClearDescs();

  /// \brief Constructs the instance data objects, within the pre-allocated memory block.
  void Construct(ezByteBlobPtr blobPtr) const;

  /// \brief Destructs the instance data objects.
  void Destruct(ezByteBlobPtr blobPtr) const;

  /// \brief Allocates memory and constructs the instance data objects inside it. The returned ezBlob must be stored somewhere.
  [[nodiscard]] ezBlob AllocateAndConstruct() const;

  /// \brief Destructs and deallocates the instance data objects and the given memory block.
  void DestructAndDeallocate(ezBlob& ref_blob) const;

  /// \brief The total size in bytes taken up by all instance data objects that were added.
  ezUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

  /// \brief Retrieves a void pointer to the instance data within the given blob at the given offset, or nullptr if the offset is invalid.
  EZ_ALWAYS_INLINE static void* GetInstanceData(const ezByteBlobPtr& blobPtr, ezUInt32 uiOffset)
  {
    return (uiOffset != ezInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
  }

private:
  ezDynamicArray<ezInstanceDataDesc> m_Descs;
  ezUInt32 m_uiTotalDataSize = 0;
};
