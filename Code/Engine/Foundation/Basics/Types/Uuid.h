
#pragma once

#include <Foundation/Basics.h>

class ezStreamReaderBase;
class ezStreamWriterBase;

/// \brief This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
///
/// To generate new Uuids use the ezUuidGenerator.
/// \sa ezUuidGenerator
class EZ_FOUNDATION_DLL ezUuid
{
public:

  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor. Constructed Uuid will be invalid. [tested]
  EZ_FORCE_INLINE ezUuid();

  /// \brief Comparision operator. [tested]
  EZ_FORCE_INLINE bool operator == (const ezUuid& Other) const;

  /// \brief Comparision operator. [tested]
  EZ_FORCE_INLINE bool operator != (const ezUuid& Other) const;

  /// \brief Returns true if this is a valid Uuid (e.g. generated with a Uuid generator, or properly deserialized)
  EZ_FORCE_INLINE bool IsValid() const;

private:

  friend class ezUuidGenerator;
  friend EZ_FOUNDATION_DLL void operator>> (ezStreamReaderBase& Stream, ezUuid& Value);
  friend EZ_FOUNDATION_DLL void operator<< (ezStreamWriterBase& Stream, const ezUuid& Value);

  /// \brief Constructor which fills the Uuid with actual data.
  ///
  /// Private so only special systems (like the ezUuidGenerator) can construct valid Uuid objects (and thus ensure uniqueness).
  EZ_FORCE_INLINE ezUuid(ezUInt64 uiHigh, ezUInt64 uiLow);

  ezUInt64 m_uiHigh;
  ezUInt64 m_uiLow;
};

#include <Foundation/Basics/Types/Implementation/Uuid_inl.h>
