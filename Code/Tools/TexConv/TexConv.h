#pragma once

#include <Foundation/Application/Application.h>

class ezStreamWriter;

class ezTexConv : public ezApplication
{
public:
  using SUPER = ezApplication;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(const char* szKey, ezInt32 iVal)
      : m_szKey(szKey)
      , m_iEnumValue(iVal)
    {
    }

    const char* m_szKey;
    ezInt32 m_iEnumValue = -1;
  };

  ezTexConv();

public:
  virtual Execution Run() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  ezResult ParseCommandLine();
  ezResult ParseOutputType();
  ezResult DetectOutputFormat();
  ezResult ParseInputFiles();
  ezResult ParseOutputFiles();
  ezResult ParseChannelMappings();
  ezResult ParseChannelSliceMapping(ezInt32 iSlice);
  ezResult ParseChannelMappingConfig(ezTexConvChannelMapping& out_mapping, const char* szCfg, ezInt32 iChannelIndex, bool bSingleChannel);
  ezResult ParseUsage();
  ezResult ParseMipmapMode();
  ezResult ParseTargetPlatform();
  ezResult ParseCompressionMode();
  ezResult ParseWrapModes();
  ezResult ParseFilterModes();
  ezResult ParseResolutionModifiers();
  ezResult ParseMiscOptions();
  ezResult ParseAssetHeader();
  ezResult ParseBumpMapFilter();

  ezResult ParseUIntOption(const char* szOption, ezInt32 iMinValue, ezInt32 iMaxValue, ezUInt32& ref_uiResult) const;
  ezResult ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& ref_iResult) const;
  void PrintOptionValues(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  void PrintOptionValuesHelp(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  bool ParseFile(const char* szOption, ezString& ref_sResult) const;

  bool IsTexFormat() const;
  ezResult WriteTexFile(ezStreamWriter& inout_stream, const ezImage& image);
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
};
