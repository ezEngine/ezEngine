#pragma once

#include <Core/Application/Application.h>

class ezTexConv2 : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezTexConv2();

  virtual ApplicationExecution Run() override;
  virtual void BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  ezResult ParseCommandLine();
  ezResult ParseInputFiles();
  ezResult ParseOutputFiles();
  ezResult ParseChannelMappings();
  ezResult ParseChannelSliceMapping(ezInt32 iSlice);
  ezResult ParseChannelMappingConfig(ezTexConvChannelMapping& out_Mapping, const char* cfg, ezInt32 iChannelIndex, bool bSingleChannel);

  ezString m_sOutputFile;
  ezString m_sOutputThumbnailFile;
  ezString m_sOutputLowResFile;

  ezTexConvProcessor m_Processor;
};

