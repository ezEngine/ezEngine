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

  enum class OutputFileType
  {
    UNKNOWN,
    DDS,
    TGA,
    PNG,
    EZ2D,
    EZ3D,
    EZCUBE,
    EZDECAL,
    EZRENDERTARGET,
  };

  typedef ezApplication SUPER;

  ezTexConv2();

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

  ezResult ParseStringOption(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed, ezInt32& iResult) const;
  void PrintOptionValues(const char* szOption, const ezDynamicArray<KeyEnumValuePair>& allowed) const;

  ezString m_sOutputFile;
  ezString m_sOutputThumbnailFile;
  ezString m_sOutputLowResFile;

  bool m_bOutputSupports2D = false;
  bool m_bOutputSupports3D = false;
  bool m_bOutputSupportsCube = false;
  bool m_bOutputSupportsRenderTarget = false;
  bool m_bOutputSupportsDecal = false;
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
