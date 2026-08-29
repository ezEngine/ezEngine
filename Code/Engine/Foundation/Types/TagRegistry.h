
#pragma once

class ezHashedString;
class ezTempHashedString;
class ezTag;
class ezStreamWriter;
class ezStreamReader;

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/Mutex.h>

/// The tag registry for tags in tag sets.
///
/// Normal usage of the tag registry is to get the global tag registry instance via ezTagRegistry::GetGlobalRegistry()
/// and to use this instance to register and get tags.
/// Certain special cases (e.g. tests) may actually need their own instance of the tag registry.
/// Note however that tags which were registered with one registry shouldn't be used with tag sets filled
/// with tags from another registry since there may be conflicting tag assignments.
/// The tag registry registration and tag retrieval functions are thread safe due to a mutex.
class EZ_FOUNDATION_DLL ezTagRegistry
{
public:
  ezTagRegistry();

  static ezTagRegistry& GetGlobalRegistry();

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const ezTag& RegisterTag(ezStringView sTagString); // [tested]

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const ezTag& RegisterTag(const ezHashedString& sTagString); // [tested]

  /// Searches for a tag with the given name and returns a pointer to it
  const ezTag* GetTagByName(const ezTempHashedString& sTagString) const; // [tested]

  /// Searches for a tag with the given murmur hash. This function is only for backwards compatibility.
  const ezTag* GetTagByMurmurHash(ezUInt32 uiMurmurHash) const;

  /// Returns the tag with the given index.
  const ezTag* GetTagByIndex(ezUInt32 uiIndex) const;

  /// Returns the number of registered tags.
  ezUInt32 GetNumTags() const;

  /// Loads the saved state and integrates it into this registry. Does not discard previously registered tag information. This function is only
  /// for backwards compatibility.
  ezResult Load(ezStreamReader& inout_stream);

protected:
  mutable ezMutex m_TagRegistryMutex;

  ezMap<ezTempHashedString, ezTag> m_RegisteredTags;
  ezDeque<ezTag*> m_TagsByIndex;
};
