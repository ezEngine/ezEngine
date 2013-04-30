
#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/HybridArray.h>

// Standard operators for overloads of common data types

/// bool versions

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, bool bValue)
{
  ezUInt8 uiValue = bValue ? 1 : 0;
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, bool& bValue)
{
  ezUInt8 uiValue = 0;
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  bValue = (uiValue != 0);
  return Stream;
}

/// unsigned int versions

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezUInt8 uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezUInt8& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezUInt16 uiValue)
{
  Stream.WriteWordValue(&uiValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezUInt16& uiValue)
{
  Stream.ReadWordValue(&uiValue);
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezUInt32 uiValue)
{
  Stream.WriteDWordValue(&uiValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezUInt32& uiValue)
{
  Stream.ReadDWordValue(&uiValue);
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezUInt64 uiValue)
{
  Stream.WriteQWordValue(&uiValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezUInt64& uiValue)
{
  Stream.ReadQWordValue(&uiValue);
  return Stream;
}

/// signed int versions

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezInt8 iValue)
{
  Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(&iValue), sizeof(ezInt8));
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezInt8& iValue)
{
  Stream.ReadBytes(reinterpret_cast<ezUInt8*>(&iValue), sizeof(ezInt8));
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezInt16 iValue)
{
  Stream.WriteWordValue(&iValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezInt16& iValue)
{
  Stream.ReadWordValue(&iValue);
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezInt32 iValue)
{
  Stream.WriteDWordValue(&iValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezInt32& iValue)
{
  Stream.ReadDWordValue(&iValue);
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, ezInt64 iValue)
{
  Stream.WriteQWordValue(&iValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezInt64& iValue)
{
  Stream.ReadQWordValue(&iValue);
  return Stream;
}

/// float and double versions

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, float fValue)
{
  Stream.WriteDWordValue(&fValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, float& fValue)
{
  Stream.ReadDWordValue(&fValue);
  return Stream;
}

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, double fValue)
{
  Stream.WriteQWordValue(&fValue);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, double& fValue)
{
  Stream.ReadQWordValue(&fValue);
  return Stream;
}

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const char* szValue)
{
  ezUInt32 uiLength = 0;

  if(szValue != NULL)
    uiLength = static_cast<ezUInt32>(strlen(szValue));

  Stream << uiLength;

  if(uiLength > 0)
    Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(szValue), uiLength);

  return Stream;
}

// ezHybridString

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezHybridString<Size, AllocatorWrapper>& sValue)
{
  return Stream << sValue.GetData();
}

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezHybridString<Size, AllocatorWrapper>& sValue)
{
  ezUInt32 uiLength = 0;

  Stream >> uiLength;

  if(uiLength > 0)
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

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezStringBuilder& sValue)
{
  return Stream << sValue.GetData();
}

template<ezUInt16 Size, typename AllocatorWrapper>
inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezStringBuilder& sValue)
{
  ezUInt32 uiLength = 0;

  Stream >> uiLength;

  if(uiLength > 0)
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





