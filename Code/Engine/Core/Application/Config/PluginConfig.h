#pragma once

#include <Core/Basics.h>
#include <Core/Application/Config/ApplicationConfig.h>

class EZ_CORE_DLL ezApplicationPluginConfig : public ezApplicationConfig
{
public:
  virtual ezResult Save() override;
  virtual void Load() override;

  virtual void Apply() override;

  struct EZ_CORE_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    ezString m_sRelativePath;
    ezSet<ezString> m_sDependecyOf;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  ezHybridArray<PluginConfig, 8> m_Plugins;
};


typedef ezApplicationPluginConfig::PluginConfig ezApplicationPluginConfig_PluginConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationPluginConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationPluginConfig_PluginConfig);
