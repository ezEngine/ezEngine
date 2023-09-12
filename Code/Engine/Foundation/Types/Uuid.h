
#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

class ezStreamReader;
class ezStreamWriter;

/// \brief This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
class EZ_FOUNDATION_DLL ezUuid
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor. Constructed Uuid will be invalid.
  EZ_ALWAYS_INLINE ezUuid(); // [tested]

  /// \brief Constructs the Uuid from existing values
  EZ_ALWAYS_INLINE ezUuid(ezUInt64 uiLow, ezUInt64 uiHigh)
  {
    m_uiLow = uiLow;
    m_uiHigh = uiHigh;
  }

  /// \brief Comparison operator. [tested]
  EZ_ALWAYS_INLINE bool operator==(const ezUuid& other) const;

  /// \brief Comparison operator. [tested]
  EZ_ALWAYS_INLINE bool operator!=(const ezUuid& other) const;

  /// \brief Comparison operator.
  EZ_ALWAYS_INLINE bool operator<(const ezUuid& other) const;

  /// \brief Returns true if this is a valid Uuid.
  EZ_ALWAYS_INLINE bool IsValid() const;

  /// \brief Returns an invalid UUID.
  [[nodiscard]] EZ_ALWAYS_INLINE static ezUuid MakeInvalid() { return ezUuid(0, 0); }

  /// \brief Returns a new Uuid.
  [[nodiscard]] static ezUuid MakeUuid();

  /// \brief Returns the internal 128 Bit of data
  void GetValues(ezUInt64& ref_uiLow, ezUInt64& ref_uiHigh) const
  {
    ref_uiHigh = m_uiHigh;
    ref_uiLow = m_uiLow;
  }

  /// \brief Creates a uuid from a string. The result is always the same for the same string.
  [[nodiscard]] static ezUuid MakeStableUuidFromString(ezStringView sString);

  /// \brief Creates a uuid from an integer. The result is always the same for the same input.
  [[nodiscard]] static ezUuid MakeStableUuidFromInt(ezInt64 iInt);

  /// \brief Adds the given seed value to this guid, creating a new guid. The process is reversible.
  EZ_ALWAYS_INLINE void CombineWithSeed(const ezUuid& seed);

  /// \brief Subtracts the given seed from this guid, restoring the original guid.
  EZ_ALWAYS_INLINE void RevertCombinationWithSeed(const ezUuid& seed);

  /// \brief Combines two guids using hashing, irreversible and order dependent.
  EZ_ALWAYS_INLINE void HashCombine(const ezUuid& hash);

private:
  friend EZ_FOUNDATION_DLL_FRIEND void operator>>(ezStreamReader& inout_stream, ezUuid& ref_value);
  friend EZ_FOUNDATION_DLL_FRIEND void operator<<(ezStreamWriter& inout_stream, const ezUuid& value);

  ezUInt64 m_uiHigh;
  ezUInt64 m_uiLow;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
