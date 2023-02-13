#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline ezStreamWriter& operator<<(ezStreamWriter& stream, bool bValue)
{
  ezUInt8 uiValue = bValue ? 1 : 0;
  stream.WriteBytes(&uiValue, sizeof(ezUInt8)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, bool& bValue)
{
  ezUInt8 uiValue = 0;
  EZ_VERIFY(stream.ReadBytes(&uiValue, sizeof(ezUInt8)) == sizeof(ezUInt8), "End of stream reached.");
  bValue = (uiValue != 0);
  return stream;
}

/// unsigned int versions

inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezUInt8 uiValue)
{
  stream.WriteBytes(&uiValue, sizeof(ezUInt8)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezUInt8& uiValue)
{
  EZ_VERIFY(stream.ReadBytes(&uiValue, sizeof(ezUInt8)) == sizeof(ezUInt8), "End of stream reached.");
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezUInt8* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezUInt8) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezUInt8* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt8) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezUInt16 uiValue)
{
  stream.WriteWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezUInt16& uiValue)
{
  stream.ReadWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezUInt16* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezUInt16) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezUInt16* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt16) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezUInt32 uiValue)
{
  stream.WriteDWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezUInt32& uiValue)
{
  stream.ReadDWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezUInt32* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezUInt32) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezUInt32* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt32) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezUInt64 uiValue)
{
  stream.WriteQWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezUInt64& uiValue)
{
  stream.ReadQWordValue(&uiValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezUInt64* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezUInt64) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezUInt64* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt64) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

/// signed int versions

inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezInt8 iValue)
{
  stream.WriteBytes(reinterpret_cast<const ezUInt8*>(&iValue), sizeof(ezInt8)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezInt8& iValue)
{
  EZ_VERIFY(stream.ReadBytes(reinterpret_cast<ezUInt8*>(&iValue), sizeof(ezInt8)) == sizeof(ezInt8), "End of stream reached.");
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezInt8* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezInt8) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezInt8* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt8) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezInt16 iValue)
{
  stream.WriteWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezInt16& iValue)
{
  stream.ReadWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezInt16* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezInt16) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezInt16* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt16) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezInt32 iValue)
{
  stream.WriteDWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezInt32& iValue)
{
  stream.ReadDWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezInt32* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezInt32) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezInt32* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt32) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, ezInt64 iValue)
{
  stream.WriteQWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezInt64& iValue)
{
  stream.ReadQWordValue(&iValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezInt64* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezInt64) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, ezInt64* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt64) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


/// float and double versions

inline ezStreamWriter& operator<<(ezStreamWriter& stream, float fValue)
{
  stream.WriteDWordValue(&fValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, float& fValue)
{
  stream.ReadDWordValue(&fValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const float* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, float* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& stream, double fValue)
{
  stream.WriteQWordValue(&fValue).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, double& fValue)
{
  stream.ReadQWordValue(&fValue).AssertSuccess();
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const double* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& stream, double* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& stream, const char* szValue);
EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& stream, ezStringView sValue);

// ezHybridString

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezHybridString<Size, AllocatorWrapper>& sValue)
{
  stream.WriteString(sValue.GetView()).AssertSuccess();
  return stream;
}

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezHybridString<Size, AllocatorWrapper>& sValue)
{
  ezStringBuilder builder;
  stream.ReadString(builder).AssertSuccess();
  sValue = std::move(builder);

  return stream;
}

// ezStringBuilder

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& stream, const ezStringBuilder& sValue);
EZ_FOUNDATION_DLL ezStreamReader& operator>>(ezStreamReader& stream, ezStringBuilder& sValue);

// ezEnum

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezEnum<T>& value)
{
  stream << value.GetValue();

  return stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  stream >> storedValue;
  value.SetValue(storedValue);

  return stream;
}

// ezBitflags

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezBitflags<T>& value)
{
  stream << value.GetValue();

  return stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  stream >> storedValue;
  value.SetValue(storedValue);

  return stream;
}
