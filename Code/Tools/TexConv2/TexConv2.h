#pragma once

#include <Core/Application/Application.h>

class ezTexConv2 : public ezApplication
{
public:

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* key, ezInt32 val)
        : m_szKey(key)
        , m_iEnumValue(val)
    {
    }

    const char* m_szKey;
    ezInt32 m_iEnumValue = -1;
  };

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
  ezResult ParseUsage();
  ezResult ParseMipmapMode();

  ezResult ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& iResult) const;

  ezString m_sOutputFile;
  ezString m_sOutputThumbnailFile;
  ezString m_sOutputLowResFile;

  ezTexConvProcessor m_Processor;


  ezDynamicArray<KeyEnumValuePair> m_AllowedUsages;
  ezDynamicArray<KeyEnumValuePair> m_AllowedMimapModes;
};
