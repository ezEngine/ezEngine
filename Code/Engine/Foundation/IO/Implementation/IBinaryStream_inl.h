
#pragma once

#if defined(EZ_BIG_ENDIAN)

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp;

  ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt16*>(pWordValue) = ezEndianHelper::Switch(uiTemp);

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp;

  ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt32*>(pDWordValue) = ezEndianHelper::Switch(uiTemp);

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp;

  ReadBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<ezUInt64*>(pQWordValue) = ezEndianHelper::Switch(uiTemp);

  return sizeof(T);
}



template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ezUInt16 uiTemp = *reinterpret_cast<const ezUInt16*>(pWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ezUInt32 uiTemp = *reinterpret_cast<const ezUInt16*>(pDWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ezUInt64 uiTemp = *reinterpret_cast<const ezUInt64*>(pQWordValue);
  uiTemp = ezEndianHelper::Switch(uiTemp);

  WriteBytes(reinterpret_cast<ezUInt8*>(&uiTemp), sizeof(T));

  return sizeof(T);
}

#else

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadWordValue(T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  ReadBytes(reinterpret_cast<ezUInt8*>(pWordValue), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadDWordValue(T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  ReadBytes(reinterpret_cast<ezUInt8*>(pDWordValue), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamReader::ReadQWordValue(T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  ReadBytes(reinterpret_cast<ezUInt8*>(pQWordValue), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteWordValue(const T* pWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt16));

  WriteBytes(reinterpret_cast<const ezUInt8*>(pWordValue), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt32));

  WriteBytes(reinterpret_cast<const ezUInt8*>(pDWordValue), sizeof(T));

  return sizeof(T);
}

template <typename T> 
ezUInt64 ezIBinaryStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  EZ_CHECK_AT_COMPILETIME(sizeof(T) == sizeof(ezUInt64));

  WriteBytes(reinterpret_cast<const ezUInt8*>(pQWordValue), sizeof(T));

  return sizeof(T);
}

#endif