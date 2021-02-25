
#pragma once

#include <Foundation/Basics.h>

/// \brief Wrapper class to provide type safety for string hashes
struct ezStringHash
{
  constexpr ezStringHash() = default;

  EZ_ALWAYS_INLINE constexpr ezStringHash(ezUInt64 uiValue)
    : m_uiValue(uiValue)
  {
  }

  EZ_ALWAYS_INLINE constexpr operator ezUInt64() const { return m_uiValue; }
  EZ_ALWAYS_INLINE constexpr operator ezInt64() const { return m_uiValue; }

  EZ_ALWAYS_INLINE constexpr bool operator==(const ezStringHash& other) const { return m_uiValue == other.m_uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator!=(const ezStringHash& other) const { return m_uiValue != other.m_uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator<(const ezStringHash& other) const { return m_uiValue < other.m_uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator>(const ezStringHash& other) const { return m_uiValue > other.m_uiValue; }

  EZ_ALWAYS_INLINE constexpr bool operator==(ezUInt64 uiValue) const { return m_uiValue == uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator!=(ezUInt64 uiValue) const { return m_uiValue != uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator<(ezUInt64 uiValue) const { return m_uiValue < uiValue; }
  EZ_ALWAYS_INLINE constexpr bool operator>(ezUInt64 uiValue) const { return m_uiValue > uiValue; }

private:
  ezStringHash(ezUInt32 uiValue);
  operator ezUInt32() const = delete;
  bool operator==(ezUInt32 uiValue) const = delete;
  bool operator!=(ezUInt32 uiValue) const = delete;

  ezUInt64 m_uiValue = 0;
};

EZ_ALWAYS_INLINE bool operator==(ezUInt64 uiHashValue, ezStringHash hash)
{
  return hash == uiHashValue;
}

EZ_ALWAYS_INLINE bool operator!=(ezUInt64 uiHashValue, ezStringHash hash)
{
  return hash != uiHashValue;
}

bool operator==(ezUInt32 uiHashValue, ezStringHash hash) = delete;
bool operator!=(ezUInt32 uiHashValue, ezStringHash hash) = delete;
