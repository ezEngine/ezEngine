
#pragma once

#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Containers/HybridArray.h>

class ezTag;
typedef ezUInt64 ezTagSetBlockStorage;

/// \brief A dynamic collection of tags featuring fast lookups.
///
/// This class can be used to store a (dynamic) collection of tags. Tags are registered within
/// the global tag registry and allocated a bit index. The tag set allows comparatively fast lookups
/// to check if a given tag is in the set or not.
/// Adding a tag may have some overhead depending whether the block storage for the tag
/// bit indices needs to be expanded or not (if the storage needs to be expanded the hybrid array will be resized).
/// Typical storage requirements for a given tag set instance should be small since the block storage is a sliding
/// window. The standard class which can be used is ezTagSet, usage of ezTagSetTemplate is only necessary
/// if the allocator needs to be overridden.
template<typename BlockStorageAllocator = ezDefaultAllocatorWrapper> class ezTagSetTemplate
{
public:

  EZ_FORCE_INLINE ezTagSetTemplate();

  /// \brief Adds the given tag to the set.
  EZ_FORCE_INLINE void Set(const ezTag& Tag); // [tested]

  /// \brief Removes the given tag.
  EZ_FORCE_INLINE void Clear(const ezTag& Tag); // [tested]

  /// \brief Returns true, if the given tag is in the set.
  EZ_FORCE_INLINE bool IsSet(const ezTag& Tag) const; // [tested]

  /// \brief Returns true if this tag set contains any tag set in the given other tag set.
  EZ_FORCE_INLINE bool IsAnySet(const ezTagSetTemplate& OtherSet) const; // [tested]

  EZ_FORCE_INLINE bool IsEmpty() const;

private:

  EZ_FORCE_INLINE bool IsTagInAllocatedRange(const ezTag& Tag) const;

  EZ_FORCE_INLINE void Reallocate(ezUInt32 uiNewTagBlockStart, ezUInt32 uiNewMaxBlockIndex);

  ezHybridArray<ezTagSetBlockStorage, 1, BlockStorageAllocator> m_TagBlocks;

  ezUInt32 m_uiTagBlockStart;

};

/// Default tag set, uses ezDefaultAllocatorWrapper for allocations.
typedef ezTagSetTemplate<> ezTagSet;

#include <Foundation/Types/Implementation/TagSet_inl.h>