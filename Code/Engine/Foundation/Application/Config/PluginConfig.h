#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class EZ_FOUNDATION_DLL ezApplicationPluginConfig
{
public:
  ezApplicationPluginConfig();

  ezResult Save(const char* szConfigPath = ":project/Plugins.ddl") const;
  void Load(const char* szConfigPath = ":project/Plugins.ddl");
  void Apply();

  struct EZ_FOUNDATION_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    ezString m_sAppDirRelativePath;
    bool m_bLoadCopy = false;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  mutable ezHybridArray<PluginConfig, 8> m_Plugins;
};


using ezApplicationPluginConfig_PluginConfig = ezApplicationPluginConfig::PluginConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezApplicationPluginConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezApplicationPluginConfig_PluginConfig);
