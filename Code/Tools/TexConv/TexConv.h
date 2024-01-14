#pragma once

#include <Foundation/Application/Application.h>
#include <Texture/TexConv/TexComparer.h>

class ezStreamWriter;

struct ezTexConvMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Convert,
    Compare,

    Default = Convert
  };
};

class ezTexConv : public ezApplication
{
public:
  using SUPER = ezApplication;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(ezStringView sKey, ezInt32 iVal)
      : m_sKey(sKey)
      , m_iEnumValue(iVal)
    {
    }

    ezStringView m_sKey;
    ezInt32 m_iEnumValue = -1;
  };

  ezTexConv();

public:
  virtual Execution Run() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  ezResult ParseCommandLine();
  ezResult ParseMode();
  ezResult ParseCompareMode();
  ezResult ParseOutputType();
  ezResult DetectOutputFormat();
  ezResult ParseInputFiles();
  ezResult ParseOutputFiles();
  ezResult ParseChannelMappings();
  ezResult ParseChannelSliceMapping(ezInt32 iSlice);
  ezResult ParseChannelMappingConfig(ezTexConvChannelMapping& out_mapping, ezStringView sCfg, ezInt32 iChannelIndex, bool bSingleChannel);
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

  ezResult ParseUIntOption(ezStringView sOption, ezInt32 iMinValue, ezInt32 iMaxValue, ezUInt32& ref_uiResult) const;
  ezResult ParseStringOption(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& ref_iResult) const;
  void PrintOptionValues(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  void PrintOptionValuesHelp(ezStringView sOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;
  bool ParseFile(ezStringView sOption, ezString& ref_sResult) const;

  bool IsTexFormat() const;
  ezResult WriteTexFile(ezStreamWriter& inout_stream, const ezImage& image);
  ezResult WriteOutputFile(ezStringView sFile, const ezImage& image);

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

  ezEnum<ezTexConvMode> m_Mode;
  ezTexConvProcessor m_Processor;

  // Comparer specific

  ezTexComparer m_Comparer;
  ezString m_sHtmlTitle;
};
