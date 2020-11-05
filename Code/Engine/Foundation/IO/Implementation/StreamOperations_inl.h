#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, bool bValue)
{
  ezUInt8 uiValue = bValue ? 1 : 0;
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8)).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, bool& bValue)
{
  ezUInt8 uiValue = 0;
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  bValue = (uiValue != 0);
  return Stream;
}

/// unsigned int versions

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezUInt8 uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(ezUInt8)).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezUInt8& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(ezUInt8));
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezUInt16 uiValue)
{
  Stream.WriteWordValue(&uiValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezUInt16& uiValue)
{
  Stream.ReadWordValue(&uiValue).IgnoreResult();
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezUInt32 uiValue)
{
  Stream.WriteDWordValue(&uiValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezUInt32& uiValue)
{
  Stream.ReadDWordValue(&uiValue).IgnoreResult();
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezUInt64 uiValue)
{
  Stream.WriteQWordValue(&uiValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezUInt64& uiValue)
{
  Stream.ReadQWordValue(&uiValue).IgnoreResult();
  return Stream;
}

/// signed int versions

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezInt8 iValue)
{
  Stream.WriteBytes(reinterpret_cast<const ezUInt8*>(&iValue), sizeof(ezInt8)).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezInt8& iValue)
{
  Stream.ReadBytes(reinterpret_cast<ezUInt8*>(&iValue), sizeof(ezInt8));
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezInt16 iValue)
{
  Stream.WriteWordValue(&iValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezInt16& iValue)
{
  Stream.ReadWordValue(&iValue).IgnoreResult();
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezInt32 iValue)
{
  Stream.WriteDWordValue(&iValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezInt32& iValue)
{
  Stream.ReadDWordValue(&iValue).IgnoreResult();
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, ezInt64 iValue)
{
  Stream.WriteQWordValue(&iValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, ezInt64& iValue)
{
  Stream.ReadQWordValue(&iValue).IgnoreResult();
  return Stream;
}

/// float and double versions

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, float fValue)
{
  Stream.WriteDWordValue(&fValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, float& fValue)
{
  Stream.ReadDWordValue(&fValue).IgnoreResult();
  return Stream;
}

inline ezStreamWriter& operator<<(ezStreamWriter& Stream, double fValue)
{
  Stream.WriteQWordValue(&fValue).IgnoreResult();
  return Stream;
}

inline ezStreamReader& operator>>(ezStreamReader& Stream, double& fValue)
{
  Stream.ReadQWordValue(&fValue).IgnoreResult();
  return Stream;
}

// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& Stream, const char* szValue);

// ezHybridString

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezHybridString<Size, AllocatorWrapper>& sValue)
{
  Stream.WriteString(sValue.GetView()).IgnoreResult();
  return Stream;
}

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamReader& operator>>(ezStreamReader& Stream, ezHybridString<Size, AllocatorWrapper>& sValue)
{
  ezStringBuilder builder;
  Stream.ReadString(builder).IgnoreResult();
  sValue = std::move(builder);

  return Stream;
}

// ezStringBuilder

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezStringBuilder& sValue);
EZ_FOUNDATION_DLL ezStreamReader& operator>>(ezStreamReader& Stream, ezStringBuilder& sValue);

// ezEnum

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezEnum<T>& value)
{
  Stream << value.GetValue();

  return Stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& Stream, ezEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  Stream >> storedValue;
  value.SetValue(storedValue);

  return Stream;
}

// ezBitflags

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& Stream, const ezBitflags<T>& value)
{
  Stream << value.GetValue();

  return Stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& Stream, ezBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  Stream >> storedValue;
  value.SetValue(storedValue);

  return Stream;
}
