#pragma once

#include <Core/Basics.h>
#include <Core/Application/Config/ApplicationConfig.h>

class EZ_CORE_DLL ezApplicationFileSystemConfig
{
public:
  ezResult Save();
  void Load();

  /// \brief Sets up the data directories that were configured or loaded into this object
  void Apply();

  /// \brief Removes all data directories that were set up by any call to ezApplicationFileSystemConfig::Apply()
  static void Clear();

  ezResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    ezString m_sDataDirSpecialPath;
    ezString m_sRootName;
    bool m_bWritable; ///< Whether the directory is going to be mounted for writing
    bool m_bHardCodedDependency; ///< If set to true, this indicates that it may not be removed by the user (in a config dialog)

    DataDirConfig()
    {
      m_bWritable = false;
      m_bHardCodedDependency = false;
    }

    bool operator==(const DataDirConfig& rhs) const { return m_bWritable == rhs.m_bWritable && m_sDataDirSpecialPath == rhs.m_sDataDirSpecialPath && m_sRootName == rhs.m_sRootName;  }
  };

  bool operator==(const ezApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  ezHybridArray<DataDirConfig, 4> m_DataDirs;
};


typedef ezApplicationFileSystemConfig::DataDirConfig ezApplicationFileSystemConfig_DataDirConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationFileSystemConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationFileSystemConfig_DataDirConfig);
