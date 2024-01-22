
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

class ezStreamReader;

/// \brief Serialization Context that reads de-duplicated objects from a stream and restores the pointers.
class EZ_FOUNDATION_DLL ezDeduplicationReadContext : public ezSerializationContext<ezDeduplicationReadContext>
{
  EZ_DECLARE_SERIALIZATION_CONTEXT(ezDeduplicationReadContext);

public:
  ezDeduplicationReadContext();
  ~ezDeduplicationReadContext();

  /// \brief Reads a single object inplace.
  template <typename T>
  ezResult ReadObjectInplace(ezStreamReader& inout_stream, T& ref_obj); // [tested]

  /// \brief Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& inout_stream, T*& ref_pObject,
    ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& inout_stream, ezSharedPtr<T>& ref_pObject,
    ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& inout_stream, ezUniquePtr<T>& ref_pObject,
    ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  ezResult ReadArray(ezStreamReader& inout_stream, ezArrayBase<ValueType, ArrayType>& ref_array,
    ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  ezResult ReadSet(ezStreamReader& inout_stream, ezSetBase<KeyType, Comparer>& ref_set,
    ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult ReadMap(ezStreamReader& inout_stream, ezMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode,
    ezAllocator* pKeyAllocator = ezFoundation::GetDefaultAllocator(),
    ezAllocator* pValueAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  ezResult ReadObject(ezStreamReader& stream, T& obj, ezAllocator* pAllocator); // [tested]

  ezDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>
