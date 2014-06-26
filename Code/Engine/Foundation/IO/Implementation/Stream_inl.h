
#pragma once

#if EZ_ENABLED(EZ_PLATFORM_BIG_ENDIAN)

template <typename T> 
ezResult ezStreamReaderBase::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt16*>(pWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T> 
ezResult ezStreamReaderBase::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt32*>(pDWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}

template <typename T> 
ezResult ezStreamReaderBase::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp;

  const ezUInt32 uiRead = ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt64*>(pQWordValue) = ezEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? EZ_SUCCESS : EZ_FAILURE;
}



template <typename T> 
ezResult ezStreamWriterBase::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp = *reinterpret_cast<const ezUInt16*>(pWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T> 
ezResult ezStreamWriterBase::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp = *reinterpret_cast<const ezUInt16*>(pDWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

template <typename T> 
ezResult ezStreamWriterBase::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp = *reinterpret_cast<const ezUInt64*>(pQWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T> 
ezResult ezStreamReaderBase::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T> 
ezResult ezStreamReaderBase::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T> 
ezResult ezStreamReaderBase::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  if (ReadBytes(reinterpret_cast<ezUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

template <typename T> 
ezResult ezStreamWriterBase::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pWordValue), sizeof(T));
}

template <typename T> 
ezResult ezStreamWriterBase::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pDWordValue), sizeof(T));
}

template <typename T> 
ezResult ezStreamWriterBase::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  return WriteBytes(reinterpret_cast<const ezUInt8*>(pQWordValue), sizeof(T));
}

#endif

