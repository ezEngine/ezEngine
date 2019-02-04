
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/Map.h>

class ezStreamWriter;

/// \brief Serialization Context that de-duplicates objects when writing to a stream. Duplicated objects are identified by their address and
/// only the first occurrence is written to the stream while all subsequence occurrences are just written as an index.
class EZ_FOUNDATION_DLL ezDeduplicationWriteContext : public ezSerializationContext<ezDeduplicationWriteContext>
{
public:
  ezDeduplicationWriteContext();
  ~ezDeduplicationWriteContext();

  /// \brief Writes a single object to the stream. Can be either a reference or a pointer to the object.
  template <typename T>
  ezResult WriteObject(ezStreamWriter& stream, const T& obj); // [tested]

  /// \brief Writes a single object to the stream.
  template <typename T>
  ezResult WriteObject(ezStreamWriter& stream, const ezSharedPtr<T>& pObject); // [tested]

  /// \brief Writes a single object to the stream.
  template <typename T>
  ezResult WriteObject(ezStreamWriter& stream, const ezUniquePtr<T>& pObject); // [tested]

  /// \brief Writes an array of de-duplicated objects.
  template <typename ArrayType, typename ValueType>
  ezResult WriteArray(ezStreamWriter& stream, const ezArrayBase<ValueType, ArrayType>& Array); // [tested]

  /// \brief Writes a set of de-duplicated objects.
  template <typename KeyType, typename Comparer>
  ezResult WriteSet(ezStreamWriter& stream, const ezSetBase<KeyType, Comparer>& Set); // [tested]

  enum class WriteMapMode
  {
    DedupKey,
    DedupValue,
    DedupBoth
  };

  /// \brief Writes a map. Mode controls whether key or value or both should de-duplicated.
  template <typename KeyType, typename ValueType, typename Comparer>
  ezResult WriteMap(ezStreamWriter& stream, const ezMapBase<KeyType, ValueType, Comparer>& Map, WriteMapMode mode); // [tested]

private:
  template <typename T>
  ezResult WriteObjectInternal(ezStreamWriter& stream, const T* pObject);

  ezHashTable<const void*, ezUInt32> m_Objects;
};

#include <Foundation/IO/Implementation/DeduplicationWriteContext_inl.h>

