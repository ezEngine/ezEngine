
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/Map.h>

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
  ezResult ReadObjectInplace(ezStreamReader& stream, T& obj); // [tested]

  /// \brief Reads a single object and sets the pointer to it. The given allocator is used to create the object if it doesn't exist yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& stream, T*& pObject,
                      ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the shared pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& stream, ezSharedPtr<T>& pObject,
                      ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a single object and sets the unique pointer to it. The given allocator is used to create the object if it doesn't exist
  /// yet.
  template <typename T>
  ezResult ReadObject(ezStreamReader& stream, ezUniquePtr<T>& pObject,
                      ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  ezResult ReadArray(ezStreamReader& stream, ezArrayBase<ValueType, ArrayType>& Array,
                     ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Reads a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  ezResult ReadSet(ezStreamReader& stream, ezSetBase<KeyType, Comparer>& Set,
                   ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  enum class ReadMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Reads a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult ReadMap(ezStreamReader& stream, ezMapBase<KeyType, ValueType, Comparer>& Map, ReadMapMode mode,
                   ezAllocatorBase* pKeyAllocator = ezFoundation::GetDefaultAllocator(),
                   ezAllocatorBase* pValueAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

private:
  template <typename T>
  ezResult ReadObject(ezStreamReader& stream, T& obj, ezAllocatorBase* pAllocator); // [tested]

  ezDynamicArray<void*> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationReadContext_inl.h>

