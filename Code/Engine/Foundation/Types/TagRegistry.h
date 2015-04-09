
#pragma once

class ezHashedString;
class ezTag;

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

  void RegisterTag(const char* szTagString, ezTag* ResultTag = nullptr); // [tested]
  void RegisterTag(const ezHashedString& TagString, ezTag* ResultTag = nullptr); // [tested]

  ezResult GetTag(const char* szTagString, ezTag& ResultTag); // [tested]
  ezResult GetTag(const ezHashedString& TagString, ezTag& ResultTag); // [tested]

protected:

  ezMutex m_TagRegistryMutex;

  ezMap<ezHashedString, ezTag> m_RegisteredTags;
  ezUInt32 m_uiNextTagIndex;
};