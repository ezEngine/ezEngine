
#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/HybridArray.h>

// Standard operators for overloads of common data types

/// bool versions

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, bool bValue)
{
  ezUInt8 uiValue = bValue ? 1 : 0;
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, bool& bValue)
{
  ezUInt8 uiValue = 0;
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  bValue = (uiValue != 0);
  return Stream;
}

/// unsigned int versions

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezUInt8 uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezUInt8& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezUInt16 uiValue)
{
  Stream.WriteWordValue(&uiValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezUInt16& uiValue)
{
  Stream.ReadWordValue(&uiValue);
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezUInt32 uiValue)
{
  Stream.WriteDWordValue(&uiValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezUInt32& uiValue)
{
  Stream.ReadDWordValue(&uiValue);
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezUInt64 uiValue)
{
  Stream.WriteQWordValue(&uiValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezUInt64& uiValue)
{
  Stream.ReadQWordValue(&uiValue);
  return Stream;
}

/// signed int versions

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezInt8 iValue)
{
  Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(&iValue), sizeof(ezInt8));
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezInt8& iValue)
{
  Stream.ReadBytes(reinterpret_cast<ezUInt8*>(&iValue), sizeof(ezInt8));
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezInt16 iValue)
{
  Stream.WriteWordValue(&iValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezInt16& iValue)
{
  Stream.ReadWordValue(&iValue);
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezInt32 iValue)
{
  Stream.WriteDWordValue(&iValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezInt32& iValue)
{
  Stream.ReadDWordValue(&iValue);
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, ezInt64 iValue)
{
  Stream.WriteQWordValue(&iValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezInt64& iValue)
{
  Stream.ReadQWordValue(&iValue);
  return Stream;
}

/// float and double versions

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, float fValue)
{
  Stream.WriteDWordValue(&fValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, float& fValue)
{
  Stream.ReadDWordValue(&fValue);
  return Stream;
}

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, double fValue)
{
  Stream.WriteQWordValue(&fValue);
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, double& fValue)
{
  Stream.ReadQWordValue(&fValue);
  return Stream;
}

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const char* szValue)
{
  ezUInt32 uiLength = 0;

  if (szValue != nullptr)
    uiLength = static_cast<ezUInt32>(strlen(szValue));

  Stream << uiLength;

  if (uiLength > 0)
    Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(szValue), uiLength);

  return Stream;
}

// ezHybridString

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezHybridString<Size, AllocatorWrapper>& sValue)
{
  return Stream << sValue.GetData();
}

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezHybridString<Size, AllocatorWrapper>& sValue)
{
  ezUInt32 uiLength = 0;

  Stream >> uiLength;

  if (uiLength > 0)
  {
    ezHybridArray<ezUInt8, 256> sTemp;
    sTemp.SetCount(uiLength + 1);

    Stream.ReadBytes(&sTemp[0], uiLength);
    sTemp[uiLength] = '\0';

    sValue = reinterpret_cast<const char*>(&sTemp[0]);
  }
  else
    sValue.Clear();

  return Stream;
}

// ezStringBuilder

inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezStringBuilder& sValue)
{
  return Stream << sValue.GetData();
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezStringBuilder& sValue)
{
  ezUInt32 uiLength = 0;

  Stream >> uiLength;

  if (uiLength > 0)
  {
    ezHybridArray<ezUInt8, 256> sTemp;
    sTemp.SetCount(uiLength + 1);

    Stream.ReadBytes(&sTemp[0], uiLength);
    sTemp[uiLength] = '\0';

    sValue = reinterpret_cast<const char*>(&sTemp[0]);
  }
  else
    sValue.Clear();

  return Stream;
}





