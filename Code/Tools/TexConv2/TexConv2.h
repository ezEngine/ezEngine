#pragma once

#include <Core/Application/Application.h>

class ezStreamWriter;

class ezTexConv2 : public ezApplication
{
public:
  typedef ezApplication SUPER;

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

  ezTexConv2();

public:
  virtual ApplicationExecution Run() override;
  virtual void BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  ezResult ParseCommandLine();
  ezResult ParseOutputType();
  ezResult DetectOutputFormat();
  ezResult ParseInputFiles();
  ezResult ParseOutputFiles();
  ezResult ParseChannelMappings();
  ezResult ParseChannelSliceMapping(ezInt32 iSlice);
  ezResult ParseChannelMappingConfig(ezTexConvChannelMapping& out_Mapping, const char* cfg, ezInt32 iChannelIndex, bool bSingleChannel);
  ezResult ParseUsage();
  ezResult ParseMipmapMode();
  ezResult ParseTargetPlatform();
  ezResult ParseCompressionMode();
  ezResult ParseWrapModes();
  ezResult ParseFilterModes();
  ezResult ParseResolutionModifiers();
  ezResult ParseMiscOptions();
  ezResult ParseAssetHeader();

  ezResult ParseUIntOption(const char* szOption, ezInt32 iMinValue, ezInt32 iMaxValue, ezUInt32& uiResult) const;
  ezResult ParseFloatOption(const char* szOption, float fMinValue, float fMaxValue, float& fResult) const;
  ezResult ParseBoolOption(const char* szOption, bool& bResult) const;
  ezResult ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& iResult) const;
  void PrintOptionValues(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  void PrintOptionValuesHelp(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  bool ParseFile(const char* szOption, ezString& result) const;

  bool IsTexFormat() const;
  ezResult WriteTexFile(ezStreamWriter& stream, const ezImage& image);
  ezResult WriteOutputFile(const char* szFile, const ezImage& image);

private:
  ezString m_sOutputFile;
  ezString m_sOutputThumbnailFile;
  ezString m_sOutputLowResFile;

  bool m_bOutputSupports2D = false;
  bool m_bOutputSupports3D = false;
  bool m_bOutputSupportsCube = false;
  bool m_bOutputSupportsAtlas = false;
  bool m_bOutputSupportsMipmaps = false;
  bool m_bOutputSupportsFiltering = false;
  bool m_bOutputSupportsCompression = false;

  ezTexConvProcessor m_Processor;

  ezDynamicArray<KeyEnumValuePair> m_AllowedOutputTypes;
  ezDynamicArray<KeyEnumValuePair> m_AllowedUsages;
  ezDynamicArray<KeyEnumValuePair> m_AllowedMimapModes;
  ezDynamicArray<KeyEnumValuePair> m_AllowedPlatforms;
  ezDynamicArray<KeyEnumValuePair> m_AllowedCompressionModes;
  ezDynamicArray<KeyEnumValuePair> m_AllowedWrapModes;
  ezDynamicArray<KeyEnumValuePair> m_AllowedFilterModes;
};
