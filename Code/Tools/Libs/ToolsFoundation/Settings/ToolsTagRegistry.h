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
  /// \brief Removes all tags that are not specified as 'built-in'
  static void Clear();

  static void WriteToDDL(ezStreamWriter& inout_stream);
  static ezStatus ReadFromDDL(ezStreamReader& inout_stream);

  static bool AddTag(const ezToolsTag& tag);
  static bool RemoveTag(ezStringView sName);

  static void GetAllTags(ezHybridArray<const ezToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezHybridArray<const ezToolsTag*, 16>& out_tags);

private:
  static ezMap<ezString, ezToolsTag> s_NameToTags;
};
