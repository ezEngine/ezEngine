#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Variant.h>

struct EZ_TOOLSFOUNDATION_DLL ezToolsTag
{
  ezString m_sName;
  ezString m_sCategory;
};

class EZ_TOOLSFOUNDATION_DLL ezToolsTagRegistry
{
public:
  static void Clear();

  static void WriteToDDL(ezStreamWriter& stream);
  static ezStatus ReadFromDDL(ezStreamReader& stream);

  static bool AddTag(const ezToolsTag& tag);
  static bool RemoveTag(const char* szName);

  static void GetAllTags(ezHybridArray<const ezToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const ezArrayPtr<ezStringView>& categories, ezHybridArray<const ezToolsTag*, 16>& out_tags);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ToolsTagRegistry);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezString, ezToolsTag> m_NameToTags;
};

