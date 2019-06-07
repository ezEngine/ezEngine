#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Variant.h>

struct EZ_TOOLSFOUNDATION_DLL ezToolsTag
{
  ezToolsTag() {}
  ezToolsTag(const char* szCategory, const char* szName, bool bBuiltIn = false)
    : m_sCategory(szCategory), m_sName(szName), m_bBuiltInTag(bBuiltIn)
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

  static void WriteToDDL(ezStreamWriter& stream);
  static ezStatus ReadFromDDL(ezStreamReader& stream);

  static bool AddTag(const ezToolsTag& tag);
  static bool RemoveTag(const char* szName);

  static void GetAllTags(ezHybridArray<const ezToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezHybridArray<const ezToolsTag*, 16>& out_tags);

private:
  static ezMap<ezString, ezToolsTag> m_NameToTags;
};

