#pragma once

#if EZ_ENABLED(EZ_PLATFORM_BIG_ENDIAN)

template <typename T>
ezResult ezStreamReader::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt16*>(pWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T>
ezResult ezStreamReader::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt32*>(pDWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T>
ezResult ezStreamReader::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt64*>(pQWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}



template <typename T>
ezResult ezStreamWriter::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp = *reinterpret_cast<const ezUInt16*>(pWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp = *reinterpret_cast<const ezUInt16*>(pDWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp = *reinterpret_cast<const ezUInt64*>(pQWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T>
ezResult ezStreamReader::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamReader::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamReader::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezStreamWriter::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pWordValue), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pDWordValue), sizeof(T));
}

template <typename T>
ezResult ezStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pQWordValue), sizeof(T));
}

#endif

ezTypeVersion ezStreamReader::ReadVersion(ezTypeVersion uiExpectedMaxVersion)
{
  ezTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();

  EZ_ASSERT_ALWAYS(v <= uiExpectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, uiExpectedMaxVersion);
  EZ_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void ezStreamWriter::WriteVersion(ezTypeVersion uiVersion)
{
  EZ_ASSERT_ALWAYS(uiVersion > 0, "Version cannot be zero.");

  WriteWordValue(&uiVersion).IgnoreResult();
}


namespace ezStreamWriterUtil
{
  // single element serialization

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& stream, const T& Obj, int) -> decltype(stream << Obj, ezResult(EZ_SUCCESS))
  {
    stream << Obj;

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& stream, const T& Obj, long) -> decltype(Obj.Serialize(stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.Serialize(stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeImpl(ezStreamWriter& stream, const T& Obj, float) -> decltype(Obj.serialize(stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.serialize(stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto Serialize(ezStreamWriter& stream, const T& Obj) -> decltype(SerializeImpl(stream, Obj, 0).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return SerializeImpl(stream, Obj, 0);
  }

  // serialization of array

  template <class T>
  EZ_ALWAYS_INLINE auto SerializeArrayImpl(ezStreamWriter& stream, const T* pArray, ezUInt64 uiCount, int) -> decltype(SerializeArray(stream, pArray, uiCount), ezResult(EZ_SUCCESS))
  {
    return SerializeArray(stream, pArray, uiCount);
  }

  template <class T>
  ezResult SerializeArrayImpl(ezStreamWriter& stream, const T* pArray, ezUInt64 uiCount, long)
  {
    for (ezUInt64 i = 0; i < uiCount; ++i)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<T>(stream, pArray[i]));
    }

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE ezResult SerializeArray(ezStreamWriter& stream, const T* pArray, ezUInt64 uiCount)
  {
    return SerializeArrayImpl(stream, pArray, uiCount, 0);
  }
} // namespace ezStreamWriterUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamWriter::WriteArray(const ezArrayBase<ValueType, ArrayType>& Array)
{
  const ezUInt64 uiCount = Array.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return ezStreamWriterUtil::SerializeArray<ValueType>(*this, Array.GetArrayPtr().GetPtr(), Array.GetCount());
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamWriter::WriteArray(const ValueType (&Array)[uiSize])
{
  const ezUInt64 uiWriteSize = uiSize;
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return ezStreamWriterUtil::SerializeArray<ValueType>(*this, Array, uiSize);
}

template <typename KeyType, typename Comparer>
ezResult ezStreamWriter::WriteSet(const ezSetBase<KeyType, Comparer>& Set)
{
  const ezUInt64 uiWriteSize = Set.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : Set)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezStreamWriter::WriteMap(const ezMapBase<KeyType, ValueType, Comparer>& Map)
{
  const ezUInt64 uiWriteSize = Map.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = Map.GetIterator(); It.IsValid(); ++It)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
ezResult ezStreamWriter::WriteHashTable(const ezHashTableBase<KeyType, ValueType, Hasher>& HashTable)
{
  const ezUInt64 uiWriteSize = HashTable.GetCount();
  EZ_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = HashTable.GetIterator(); It.IsValid(); ++It)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return EZ_SUCCESS;
}

namespace ezStreamReaderUtil
{
  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& stream, T& Obj, int) -> decltype(stream >> Obj, ezResult(EZ_SUCCESS))
  {
    stream >> Obj;

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& stream, T& Obj, long) -> decltype(Obj.Deserialize(stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.Deserialize(stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeImpl(ezStreamReader& stream, T& Obj, float) -> decltype(Obj.deserialize(stream).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.deserialize(stream));
  }

  template <class T>
  EZ_ALWAYS_INLINE auto Deserialize(ezStreamReader& stream, T& Obj) -> decltype(DeserializeImpl(stream, Obj, 0).IgnoreResult(), ezResult(EZ_SUCCESS))
  {
    return DeserializeImpl(stream, Obj, 0);
  }

  // serialization of array

  template <class T>
  EZ_ALWAYS_INLINE auto DeserializeArrayImpl(ezStreamReader& stream, T* pArray, ezUInt64 uiCount, int) -> decltype(DeserializeArray(stream, pArray, uiCount), ezResult(EZ_SUCCESS))
  {
    return DeserializeArray(stream, pArray, uiCount);
  }

  template <class T>
  ezResult DeserializeArrayImpl(ezStreamReader& stream, T* pArray, ezUInt64 uiCount, long)
  {
    for (ezUInt64 i = 0; i < uiCount; ++i)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(stream, pArray[i]));
    }

    return EZ_SUCCESS;
  }

  template <class T>
  EZ_ALWAYS_INLINE ezResult DeserializeArray(ezStreamReader& stream, T* pArray, ezUInt64 uiCount)
  {
    return DeserializeArrayImpl(stream, pArray, uiCount, 0);
  }

} // namespace ezStreamReaderUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamReader::ReadArray(ezArrayBase<ValueType, ArrayType>& Array)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    Array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(Array).SetCount(static_cast<ezUInt32>(uiCount));

      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::DeserializeArray<ValueType>(*this, Array.GetData(), uiCount));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamReader::ReadArray(ValueType (&Array)[uiSize])
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (static_cast<ezUInt32>(uiCount) != uiSize)
    return EZ_FAILURE;

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::DeserializeArray<ValueType>(*this, Array, uiCount));

    return EZ_SUCCESS;
  }

  // Containers currently use 32 bit for counts internally. Value from file is too large.
  return EZ_FAILURE;
}

template <typename KeyType, typename Comparer>
ezResult ezStreamReader::ReadSet(ezSetBase<KeyType, Comparer>& Set)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    Set.Clear();

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Item;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Item));

      Set.Insert(std::move(Item));
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
ezResult ezStreamReader::ReadMap(ezMapBase<KeyType, ValueType, Comparer>& Map)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    Map.Clear();

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Key));
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Value));

      Map.Insert(std::move(Key), std::move(Value));
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
ezResult ezStreamReader::ReadHashTable(ezHashTableBase<KeyType, ValueType, Hasher>& HashTable)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < ezMath::MaxValue<ezUInt32>())
  {
    HashTable.Clear();
    HashTable.Reserve(static_cast<ezUInt32>(uiCount));

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Key));
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Value));

      HashTable.Insert(std::move(Key), std::move(Value));
    }

    return EZ_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return EZ_FAILURE;
  }
}
