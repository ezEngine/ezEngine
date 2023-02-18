#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, bool bValue)
{
  ezUInt8 uiValue = bValue ? 1 : 0;
  inout_stream.WriteBytes(&uiValue, sizeof(ezUInt8)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, bool& out_bValue)
{
  ezUInt8 uiValue = 0;
  EZ_VERIFY(inout_stream.ReadBytes(&uiValue, sizeof(ezUInt8)) == sizeof(ezUInt8), "End of stream reached.");
  out_bValue = (uiValue != 0);
  return inout_stream;
}

/// unsigned int versions

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezUInt8 uiValue)
{
  inout_stream.WriteBytes(&uiValue, sizeof(ezUInt8)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezUInt8& out_uiValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(&out_uiValue, sizeof(ezUInt8)) == sizeof(ezUInt8), "End of stream reached.");
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezUInt8* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezUInt8) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezUInt8* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezUInt16 uiValue)
{
  inout_stream.WriteWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezUInt16& ref_uiValue)
{
  inout_stream.ReadWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezUInt16* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezUInt16) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezUInt16* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezUInt32 uiValue)
{
  inout_stream.WriteDWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezUInt32& ref_uiValue)
{
  inout_stream.ReadDWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezUInt32* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezUInt32) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezUInt32* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezUInt64 uiValue)
{
  inout_stream.WriteQWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezUInt64& ref_uiValue)
{
  inout_stream.ReadQWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezUInt64* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezUInt64) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezUInt64* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezUInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

/// signed int versions

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezInt8 iValue)
{
  inout_stream.WriteBytes(reinterpret_cast<const ezUInt8*>(&iValue), sizeof(ezInt8)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezInt8& ref_iValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(reinterpret_cast<ezUInt8*>(&ref_iValue), sizeof(ezInt8)) == sizeof(ezInt8), "End of stream reached.");
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezInt8* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezInt8) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezInt8* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezInt16 iValue)
{
  inout_stream.WriteWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezInt16& ref_iValue)
{
  inout_stream.ReadWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezInt16* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezInt16) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezInt16* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezInt32 iValue)
{
  inout_stream.WriteDWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezInt32& ref_iValue)
{
  inout_stream.ReadDWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezInt32* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezInt32) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezInt32* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezInt64 iValue)
{
  inout_stream.WriteQWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezInt64& ref_iValue)
{
  inout_stream.ReadQWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezInt64* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezInt64) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, ezInt64* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


/// float and double versions

inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, float fValue)
{
  inout_stream.WriteDWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, float& ref_fValue)
{
  inout_stream.ReadDWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const float* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, float* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, double fValue)
{
  inout_stream.WriteQWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, double& ref_fValue)
{
  inout_stream.ReadQWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const double* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline ezResult DeserializeArray(ezStreamReader& inout_stream, double* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as ezString & ezStringBuilder instances)

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const char* szValue);
EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& inout_stream, ezStringView sValue);

// ezHybridString

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezHybridString<Size, AllocatorWrapper>& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

template <ezUInt16 Size, typename AllocatorWrapper>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezHybridString<Size, AllocatorWrapper>& out_sValue)
{
  ezStringBuilder builder;
  inout_stream.ReadString(builder).AssertSuccess();
  out_sValue = std::move(builder);

  return inout_stream;
}

// ezStringBuilder

EZ_FOUNDATION_DLL ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezStringBuilder& sValue);
EZ_FOUNDATION_DLL ezStreamReader& operator>>(ezStreamReader& inout_stream, ezStringBuilder& out_sValue);

// ezEnum

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezEnum<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}

// ezBitflags

template <typename T>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezBitflags<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}
