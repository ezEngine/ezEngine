#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct EZ_TOOLSFOUNDATION_DLL ezToolsTag
{
  ezToolsTag() = default;
  ezToolsTag(ezStringView sCategory, ezStringView sName, bool bBuiltIn = false)
    : m_sCategory(sCategory)
    , m_sName(sName)
    , m_bBuiltInTag(bBuiltIn)
  {
  }

  ezString m_sCategory;
  ezString m_sName;
  bool m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class EZ_TOOLSFOUNDATION_DLL ezToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'.
  static void Clear();

  /// \brief Serializes all tags to a DDL stream.
  static void WriteToDDL(ezStreamWriter& inout_stream);
  /// \brief Reads tags from a DDL stream.
  static ezStatus ReadFromDDL(ezStreamReader& inout_stream);

  /// \brief Adds a tag to the registry. Returns true if the tag was valid.
  static bool AddTag(const ezToolsTag& tag);
  /// \brief Removes a tag by name. Returns true if the tag was removed.
  static bool RemoveTag(ezStringView sName);

  /// \brief Retrieves all tags in the registry.
  static void GetAllTags(ezDynamicArray<const ezToolsTag*>& out_tags);
  /// \brief Retrieves all tags in the given categories.
  static void GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezDynamicArray<const ezToolsTag*>& out_tags);

private:
  static ezMap<ezString, ezToolsTag> s_NameToTags;
};
