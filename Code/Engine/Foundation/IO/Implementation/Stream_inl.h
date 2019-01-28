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
  ezTypeVersion v;
  ReadWordValue(&v);

  EZ_ASSERT_ALWAYS(v <= uiExpectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, uiExpectedMaxVersion);

  return v;
}

void ezStreamWriter::WriteVersion(ezTypeVersion uiVersion)
{
  WriteWordValue(&uiVersion);
}


namespace ezStreamWriterUtil
{
  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, int) -> decltype(Stream << Obj, ezResult(EZ_SUCCESS))
  {
    Stream << Obj;

    return EZ_SUCCESS;
  }

  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, long) -> decltype(Obj.Serialize(Stream), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.Serialize(Stream));
  }

  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, float) -> decltype(Obj.serialize(Stream), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.serialize(Stream));
  }

  template <class T>
  auto Serialize(ezStreamWriter& Stream, const T& Obj) -> decltype(SerializeImpl(Stream, Obj, 0), ezResult(EZ_SUCCESS))
  {
    return SerializeImpl(Stream, Obj, 0);
  }
} // namespace ezStreamWriterUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamWriter::WriteArray(const ezArrayBase<ValueType, ArrayType>& Array)
{
  const ezUInt64 uiCount = Array.GetCount();
  WriteQWordValue(&uiCount);

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, Array[i]));
  }

  return EZ_SUCCESS;
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamWriter::WriteArray(const ValueType (&Array)[uiSize])
{
  const ezUInt64 uiWriteSize = uiSize;
  WriteQWordValue(&uiWriteSize);

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiSize); ++i)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, Array[i]));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezStreamWriter::WriteSet(const ezSetBase<KeyType, Comparer>& Set)
{
  const ezUInt64 uiWriteSize = Set.GetCount();
  WriteQWordValue(&uiWriteSize);

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
  WriteQWordValue(&uiWriteSize);

  for (auto It = Map.GetIterator(); It.IsValid(); ++It)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return EZ_SUCCESS;
}

namespace ezStreamReaderUtil
{
  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, int) -> decltype(Stream >> Obj, ezResult(EZ_SUCCESS))
  {
    Stream >> Obj;

    return EZ_SUCCESS;
  }

  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, long) -> decltype(Obj.Deserialize(Stream), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.Deserialize(Stream));
  }

  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, float) -> decltype(Obj.deserialize(Stream), ezResult(EZ_SUCCESS))
  {
    return ezToResult(Obj.deserialize(Stream));
  }

  template <class T>
  auto Deserialize(ezStreamReader& Stream, T& Obj) -> decltype(DeserializeImpl(Stream, Obj, 0), ezResult(EZ_SUCCESS))
  {
    return DeserializeImpl(Stream, Obj, 0);
  }
} // namespace ezStreamReaderUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamReader::ReadArray(ezArrayBase<ValueType, ArrayType>& Array)
{
  ezUInt64 uiCount = 0;
  ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(Array).Reserve(uiCount);

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<ValueType>(*this, Array.ExpandAndGetRef()));
    }
  }

  return EZ_SUCCESS;
}

template <typename ValueType, ezUInt32 uiSize>
ezResult ezStreamReader::ReadArray(ValueType (&Array)[uiSize])
{
  ezUInt64 uiCount = 0;
  ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  if (static_cast<ezUInt32>(uiCount) != uiSize)
    return EZ_FAILURE;

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<ValueType>(*this, Array[i]));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezStreamReader::ReadSet(ezSetBase<KeyType, Comparer>& Set)
{
  ezUInt64 uiCount = 0;
  ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Set.Clear();

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    KeyType Item;
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize(*this, Item));

    Set.Insert(std::move(Item));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezStreamReader::ReadMap(ezMapBase<KeyType, ValueType, Comparer>& Map)
{
  ezUInt64 uiCount = 0;
  ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

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
