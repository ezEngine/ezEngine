
#pragma once

#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>

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
template<typename BlockStorageAllocator = ezDefaultAllocatorWrapper> 
class ezTagSetTemplate
{
public:
  ezTagSetTemplate();

  /// \brief Adds the given tag to the set.
  void Set(const ezTag& Tag); // [tested]

  /// \brief Removes the given tag.
  void Remove(const ezTag& Tag); // [tested]

  /// \brief Returns true, if the given tag is in the set.
  bool IsSet(const ezTag& Tag) const; // [tested]

  /// \brief Returns true if this tag set contains any tag set in the given other tag set.
  bool IsAnySet(const ezTagSetTemplate& OtherSet) const; // [tested]

  /// \brief Returns how many tags are in this set.
  ezUInt32 GetNumTagsSet() const;

  /// \brief True if the tag set never contained any tag or was cleared.
  EZ_FORCE_INLINE bool IsEmpty() const;

  /// \brief Removes all tags from the set
  void Clear();

  /// \brief Adds the tag with the given name. If the tag does not exist, it will be registered.
  void SetByName(const char* szTag);

  /// \brief Removes the given tag. If it doesn't exist, nothing happens.
  void RemoveByName(const char* szTag);

  /// \brief Checks whether the named tag is part of this set. Returns false if the tag does not exist.
  bool IsSetByName(const char* szTag) const;

  /// \brief Allows to iterate over all tags in this set
  class Iterator
  {
  public:
    Iterator(const ezTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd = false);

    /// \brief Returns a reference to the current tag
    const ezTag* operator*() const;

    /// \brief Returns whether the iterator is still pointing to a valid item
    EZ_FORCE_INLINE bool IsValid() const
    {
      return m_uiIndex != 0xFFFFFFFF;
    }

    EZ_FORCE_INLINE bool operator!=(const Iterator& rhs) const
    {
      return m_pTagSet != rhs.m_pTagSet || m_uiIndex != rhs.m_uiIndex;
    }

    /// \brief Advances the iterator to the next item
    void operator++();

  private:
    bool IsBitSet() const;

    const ezTagSetTemplate<BlockStorageAllocator>* m_pTagSet;
    ezUInt32 m_uiIndex;
  };

  /// \brief Returns an iterator to list all tags in this set
  Iterator GetIterator() const { return Iterator(this); }

  /// \brief Writes the tag set state to a stream.
  ///
  /// \note This only stores the tag hashes. The ezTagRegistry must be stored as well, to be able to restore the tags.
  void Save(ezStreamWriter& stream) const;

  /// \brief Reads the tag set state from a stream.
  ///
  /// \note This will restore all tags that could be found in the given registry.
  ///       The registry must be serialized separately.
  ///       Tags that are not found in the registry will be ignored.
  void Load(ezStreamReader& stream, const ezTagRegistry& registry);

private:
  friend class Iterator;

  EZ_FORCE_INLINE bool IsTagInAllocatedRange(const ezTag& Tag) const;

  void Reallocate(ezUInt32 uiNewTagBlockStart, ezUInt32 uiNewMaxBlockIndex);

  ezHybridArray<ezTagSetBlockStorage, 1, BlockStorageAllocator> m_TagBlocks;

  ezUInt32 m_uiTagBlockStart;

};

/// Default tag set, uses ezDefaultAllocatorWrapper for allocations.
typedef ezTagSetTemplate<> ezTagSet;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTagSet);

template <typename BlockStorageAllocator>
typename ezTagSetTemplate<BlockStorageAllocator>::Iterator cbegin(const ezTagSetTemplate<BlockStorageAllocator>& cont) { return typename ezTagSetTemplate<BlockStorageAllocator>::Iterator(&cont); }

template <typename BlockStorageAllocator>
typename ezTagSetTemplate<BlockStorageAllocator>::Iterator cend(const ezTagSetTemplate<BlockStorageAllocator>& cont) { return typename ezTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true); }

template <typename BlockStorageAllocator>
typename ezTagSetTemplate<BlockStorageAllocator>::Iterator begin(const ezTagSetTemplate<BlockStorageAllocator>& cont) { return typename ezTagSetTemplate<BlockStorageAllocator>::Iterator(&cont); }

template <typename BlockStorageAllocator>
typename ezTagSetTemplate<BlockStorageAllocator>::Iterator end(const ezTagSetTemplate<BlockStorageAllocator>& cont) { return typename ezTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true); }

#include <Foundation/Types/Implementation/TagSet_inl.h>

