#pragma once

#include <Core/Basics.h>
#include <Core/Application/Config/ApplicationConfig.h>

class EZ_CORE_DLL ezApplicationFileSystemConfig : public ezApplicationConfig
{
public:
  virtual ezResult Save() override;
  virtual void Load() override;

  virtual void Apply() override;

  struct DataDirConfig
  {
    ezString m_sRelativePath;
    bool m_bWritable;
  };

  ezHybridArray<DataDirConfig, 4> m_DataDirs;
};


