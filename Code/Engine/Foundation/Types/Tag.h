
#pragma once

#include <Foundation/Strings/HashedString.h>

typedef ezUInt64 ezTagSetBlockStorage;

/// \brief The tag class stores the necessary lookup information for a single tag which can be used in conjunction with the tag set.
///
/// A tag is the storage for a small amount of lookup information for a single tag. Instances
/// of ezTag can be used in checks with the tag set. Note that fetching information for the tag needs to access
/// the global tag registry which involves a mutex lock. It is thus
/// recommended to fetch tag instances early and reuse them for the actual tests and to avoid querying the tag registry
/// all the time (e.g. due to tag instances being kept on the stack).
class EZ_FOUNDATION_DLL ezTag
{
public:

  EZ_FORCE_INLINE ezTag();

  EZ_FORCE_INLINE bool operator == (const ezTag& rhs) const; // [tested]

  EZ_FORCE_INLINE bool operator != (const ezTag& rhs) const; // [tested]

  EZ_FORCE_INLINE bool operator < (const ezTag& rhs) const;

  EZ_FORCE_INLINE const ezString& GetTagString() const; // [tested]

  EZ_FORCE_INLINE ezUInt32 GetTagHash() const; // [tested]

  EZ_FORCE_INLINE bool IsValid() const; // [tested]

private:

  template<typename BlockStorageAllocator> friend class ezTagSetTemplate;
  friend class ezTagRegistry;

  ezHashedString m_TagString;

  ezUInt32 m_uiBitIndex;
  ezUInt32 m_uiBlockIndex;
  
  /// Stores a pre-shifted version of 1u << uiBitIndex
  ezTagSetBlockStorage m_uiPreshiftedBit;
};

#include <Foundation/Types/TagSet.h>

#include <Foundation/Types/Implementation/Tag_inl.h>