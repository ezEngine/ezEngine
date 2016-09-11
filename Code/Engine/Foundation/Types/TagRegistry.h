
#pragma once

class ezHashedString;
class ezTempHashedString;
class ezTag;
class ezStreamWriter;
class ezStreamReader;

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Containers/Map.h>

/// \brief The tag registry for tags in tag sets.
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

  /// \brief Ensures the tag with the given name exists and returns a pointer to it.
  const ezTag* RegisterTag(const char* szTagString); // [tested]

  /// \brief Ensures the tag with the given name exists and returns a pointer to it.
  const ezTag* RegisterTag(const ezHashedString& TagString); // [tested]

  /// \brief Searches for a tag with the given name and returns a pointer to it
  const ezTag* GetTagByName(const ezTempHashedString& TagString) const; // [tested]

  /// \brief Returns the tag with the given index.
  const ezTag* GetTagByIndex(ezUInt32 uiIndex) const;

  /// \brief Returns the number of registered tags.
  ezUInt32 GetNumTags() const;

  /// \brief Writes all information to the stream that is necessary to restore the registry.
  void Save(ezStreamWriter& stream) const;

  /// \brief Loads the saved state and integrates it into this registry. Does not discard previously registered tag information.
  void Load(ezStreamReader& stream);

protected:

  mutable ezMutex m_TagRegistryMutex;

  ezMap<ezTempHashedString, ezTag> m_RegisteredTags;
  ezDeque<ezTag*> m_TagsByIndex;
};