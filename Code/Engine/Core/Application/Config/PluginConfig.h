#pragma once

#include <Core/Basics.h>
#include <Core/Application/Config/ApplicationConfig.h>

class EZ_CORE_DLL ezApplicationPluginConfig : public ezApplicationConfig
{
public:
  ezApplicationPluginConfig();

  virtual ezResult Save() override;
  virtual void Load() override;

  virtual void Apply() override;

  /// \brief If enabled (default is true), Apply() will only load plugins that have the dependency '<manual>' and ignore all that do not have it.
  ///
  /// It typically makes sense only to load plugins that the user specifically asked for.
  /// Only few applications may want to additionally load plugins that are dependencies of some other
  /// plugin (editors), but NOT also requested by the user.
  void SetOnlyLoadManualPlugins(bool b) { m_bManualOnly = b; }

  struct EZ_CORE_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    ezString m_sRelativePath;
    ezSet<ezString> m_sDependecyOf;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  ezHybridArray<PluginConfig, 8> m_Plugins;

private:
  bool m_bManualOnly;
};


typedef ezApplicationPluginConfig::PluginConfig ezApplicationPluginConfig_PluginConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationPluginConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationPluginConfig_PluginConfig);
