#pragma once

#include <Core/Basics.h>
#include <Core/Application/Config/ApplicationConfig.h>

class EZ_CORE_DLL ezApplicationFileSystemConfig : public ezApplicationConfig
{
public:
  virtual ezResult Save() override;
  virtual void Load() override;

  virtual void Apply() override;

  ezResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    ezString m_sRelativePath;
    bool m_bWritable;

    bool operator==(const DataDirConfig& rhs) const { return m_bWritable == rhs.m_bWritable && m_sRelativePath == rhs.m_sRelativePath;  }
  };

  bool operator==(const ezApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  ezHybridArray<DataDirConfig, 4> m_DataDirs;
};


typedef ezApplicationFileSystemConfig::DataDirConfig ezApplicationFileSystemConfig_DataDirConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationFileSystemConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationFileSystemConfig_DataDirConfig);
