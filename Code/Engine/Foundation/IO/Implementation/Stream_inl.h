#pragma once

#if EZ_ENABLED(EZ_PLATFORM_BIG_ENDIAN)

template <typename T>
ezResult ezStreamReader::ReadWordValue(T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt16*>(pWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T>
ezResult ezStreamReader::ReadDWordValue(T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt32*>(pDWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T>
ezResult ezStreamReader::ReadQWordValue(T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt64*>(pQWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}



template <typename T>
ezResult ezStreamWriter::WriteWordValue(const T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp = *reinterpret_cast<const ezUInt16*>(pWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp = *reinterpret_cast<const ezUInt32*>(pDWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp = *reinterpret_cast<const ezUInt64*>(pQWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T>
ezResult ezStreamReader::ReadWordValue(T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt16));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamReader::ReadDWordValue(T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt32));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamReader::ReadQWordValue(T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt64));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamWriter::WriteWordValue(const T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt16));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pWordValue), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt32));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pDWordValue), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(ezUInt64));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pQWordValue), sizeof(T));
}

#endif

ezTypeVersion ezStreamReader::ReadVersion(ezTypeVersion expectedMaxVersion)
{
  ezTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();

  EZ_ASSERT_ALWAYS(v <= expectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, expectedMaxVersion);
  EZ_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void ezStreamWriter::WriteVersion(ezTypeVersion version)
{
  EZ_ASSERT_ALWAYS(version > 0, "Version cannot be zero.");

  WriteWordValue(&version).IgnoreResult();
}


namespace ezStreamWriterUtil
{
  // single element serialization

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& inout_stream, const T& obj, int) -> decltype(inout_stream << obj, ezResult(EZ_SUCCESS))
  {
    inout_stream << obj;

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& inout_stream, const T& obj, long) -> decltype(obj.Serialize(inout_stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(obj.Serialize(inout_stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& inout_stream, const T& obj, float) -> decltype(obj.serialize(inout_stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(obj.serialize(inout_stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto Serialize(ezStreamWriter& inout_stream, const T& obj) -> decltype(SerializeImpl(inout_stream, obj, 0).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return SerializeImpl(inout_stream, obj, 0);
  }

  // serialization of array

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeArrayImpl(ezStreamWriter& inout_stream, const T* pArray, ezUInt64 uiCount, int) -> decltype(SerializeArray(inout_stream, pArray, uiCount), ezResult(EZ_SUCCESS))
  {
    return SerializeArray(inout_stream, pArray, uiCount);
  }

  template <class T>
  ezResult SerializeArrayImpl(ezStreamWriter& inout_stream, const T* pArray, ezUInt64 uiCount, long)
  {
    for (ezUInt64 i = 0; i < uiCount; ++i)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<T>(inout_stream, pArray[i]));
    }

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE ezResult SerializeArray(ezStreamWriter& inout_stream, const T* pArray, ezUInt64 uiCount)
  {
    return SerializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }
} // namespace ezStreamWriterUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamWriter::WriteArray(const ezArrayBase<ValueType, ArrayType>& array)
{
  const ezUInt64 uiCount = array.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return ezStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, ezUInt16 uiSize>
ezResult ezStreamWriter::WriteArray(const ezSmallArrayBase<ValueType, uiSize>& array)
{
  const ezUInt32 uiCount = array.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));

  return ezStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamWriter::WriteArray(const ValueType (&array)[uiSize])
{
  const ezUInt64 uiWriteSize = uiSize;
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return ezStreamWriterUtil::SerializeArray<ValueType>(*this, array, uiSize);
}

template <typename KeyType, typename Comparer>
ezResult ezStreamWriter::WriteSet(const ezSetBase<KeyType, Comparer>& set)
{
  const ezUInt64 uiWriteSize = set.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezStreamWriter::WriteMap(const ezMapBase<KeyType, ValueType, Comparer>& map)
{
  const ezUInt64 uiWriteSize = map.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = map.GetIterator(); It.IsValid(); ++It)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
ezResult ezStreamWriter::WriteHashTable(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable)
{
  const ezUInt64 uiWriteSize = hashTable.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = hashTable.GetIterator(); It.IsValid(); ++It)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return EZ_SUCCESS;
}

namespace ezStreamReaderUtil
{
  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& inout_stream, T& ref_obj, int) -> decltype(inout_stream >> ref_obj, ezResult(EZ_SUCCESS))
  {
    inout_stream >> ref_obj;

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& inout_stream, T& inout_obj, long) -> decltype(inout_obj.Deserialize(inout_stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(inout_obj.Deserialize(inout_stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& inout_stream, T& inout_obj, float) -> decltype(inout_obj.deserialize(inout_stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(inout_obj.deserialize(inout_stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto Deserialize(ezStreamReader& inout_stream, T& inout_obj) -> decltype(DeserializeImpl(inout_stream, inout_obj, 0).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return DeserializeImpl(inout_stream, inout_obj, 0);
  }

  // serialization of array

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeArrayImpl(ezStreamReader& inout_stream, T* pArray, ezUInt64 uiCount, int) -> decltype(DeserializeArray(inout_stream, pArray, uiCount), ezResult(EZ_SUCCESS))
  {
    return DeserializeArray(inout_stream, pArray, uiCount);
  }

  template <class T>
  ezResult DeserializeArrayImpl(ezStreamReader& inout_stream, T* pArray, ezUInt64 uiCount, long)
  {
    for (ezUInt64 i = 0; i < uiCount; ++i)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(inout_stream, pArray[i]));
    }

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE ezResult DeserializeArray(ezStreamReader& inout_stream, T* pArray, ezUInt64 uiCount)
  {
    return DeserializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }

} // namespace ezStreamReaderUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamReader::ReadArray(ezArrayBase<ValueType, ArrayType>& inout_array)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    inout_array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(inout_array).SetCount(static_cast<ezUInt32>(uiCount));

      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::DeserializeArray<ValueType>(*this, inout_array.GetData(), uiCount));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}

template <typename ValueType, ezUInt16 uiSize, typename AllocatorWrapper>
ezResult ezStreamReader::ReadArray(ezSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array)
{
  ezUInt32 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt16>())
  {
    ref_array.Clear();

    if (uiCount > 0)
    {
      ref_array.SetCount(static_cast<ezUInt16>(uiCount));

      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::DeserializeArray<ValueType>(*this, ref_array.GetData(), uiCount));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Small array uses 16 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamReader::ReadArray(ValueType (&array)[uiSize])
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (static_cast<ezUInt32>(uiCount) != uiSize)
    return EZ_FAILURE;

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::DeserializeArray<ValueType>(*this, array, uiCount));

    return EZ_SUCCESS;
  }

  // Containers currently use 32 bit for counts internally. Value from file is too large.
  return EZ_FAILURE;
}

template <typename KeyType, typename Comparer>
ezResult ezStreamReader::ReadSet(ezSetBase<KeyType, Comparer>& inout_set)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    inout_set.Clear();

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Item;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Item));

      inout_set.Insert(std::move(Item));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezStreamReader::ReadMap(ezMapBase<KeyType, ValueType, Comparer>& inout_map)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    inout_map.Clear();

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Key));
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Value));

      inout_map.Insert(std::move(Key), std::move(Value));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Hasher>
ezResult ezStreamReader::ReadHashTable(ezHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    inout_hashTable.Clear();
    inout_hashTable.Reserve(static_cast<ezUInt32>(uiCount));

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Key));
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Value));

      inout_hashTable.Insert(std::move(Key), std::move(Value));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}
