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

namespace ezStreamWriterUtil
{
  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, int) -> decltype(Stream << Obj, void())
  {
    Stream << Obj;
  }

  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, long) -> decltype(Obj.Serialize(Stream), void())
  {
    Obj.Serialize(Stream);
  }

  template <class T>
  auto SerializeImpl(ezStreamWriter& Stream, const T& Obj, float) -> decltype(Obj.serialize(Stream), void())
  {
    Obj.serialize(Stream);
  }

  template <class T>
  auto Serialize(ezStreamWriter& Stream, const T& Obj) -> decltype(SerializeImpl(Stream, Obj, 0), void())
  {
    SerializeImpl(Stream, Obj, 0);
  }
}// namespace ezStreamWriterUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamWriter::WriteArray(const ezArrayBase<ValueType, ArrayType>& Array)
{
  const ezUInt32 uiCount = Array.GetCount();
  WriteDWordValue(&uiCount);

  for(ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezStreamWriterUtil::Serialize<ValueType>(*this, Array[i]);
  }

  return EZ_SUCCESS;
}

namespace ezStreamReaderUtil
{
  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, int) -> decltype(Stream >> Obj, void())
  {
    Stream >> Obj;
  }

  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, long) -> decltype(Obj.Deserialize(Stream), void())
  {
    Obj.Deserialize(Stream);
  }

  template <class T>
  auto DeserializeImpl(ezStreamReader& Stream, T& Obj, float) -> decltype(Obj.deserialize(Stream), void())
  {
    Obj.deserialize(Stream);
  }

  template <class T>
  auto Deserialize(ezStreamReader& Stream, T& Obj) -> decltype(DeserializeImpl(Stream, Obj, 0), void())
  {
    DeserializeImpl(Stream, Obj, 0);
  }
} // namespace ezStreamReaderUtil

template <typename ArrayType, typename ValueType>
ezResult ezStreamReader::ReadArray(ezArrayBase<ValueType, ArrayType>& Array)
{
  ezUInt32 uiCount = 0;
  ReadDWordValue(&uiCount);

  Array.Clear();

  if(uiCount > 0)
  {
    static_cast<ArrayType&>(Array).Reserve(uiCount);

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      ezStreamReaderUtil::Deserialize<ValueType>(*this, Array.ExpandAndGetRef());
    }
  }

  return EZ_SUCCESS;
}
