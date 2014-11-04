
#pragma once

#include <Foundation/Algorithm/Hashing.h>

class ezStreamReaderBase;
class ezStreamWriterBase;

/// \brief This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
class EZ_FOUNDATION_DLL ezUuid
{
public:

  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor. Constructed Uuid will be invalid. [tested]
  EZ_FORCE_INLINE ezUuid();

  /// \brief Comparison operator. [tested]
  EZ_FORCE_INLINE bool operator == (const ezUuid& Other) const;

  /// \brief Comparison operator. [tested]
  EZ_FORCE_INLINE bool operator != (const ezUuid& Other) const;

  /// \brief Comparison operator.
  EZ_FORCE_INLINE bool operator < (const ezUuid& Other) const;

  /// \brief Returns true if this is a valid Uuid.
  EZ_FORCE_INLINE bool IsValid() const;

  /// \brief Creates a new Uuid and stores is it in this object.
  void CreateNewUuid();

private:

  friend EZ_FOUNDATION_DLL void operator>> (ezStreamReaderBase& Stream, ezUuid& Value);
  friend EZ_FOUNDATION_DLL void operator<< (ezStreamWriterBase& Stream, const ezUuid& Value);

  ezUInt64 m_uiHigh;
  ezUInt64 m_uiLow;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
