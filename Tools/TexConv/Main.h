#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>
#include <TexConv/DirectXTex/DirectXTex.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Image/Formats/ImageFormatMappings.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>

using namespace DirectX;

class ezTexConv : public ezApplication
{
public:

  enum Channel
  {
    None = 0,
    Red = EZ_BIT(0),
    Green = EZ_BIT(1),
    Blue = EZ_BIT(2),
    Alpha = EZ_BIT(3),
    All = 0xFF
  };

  enum class TextureType
  {
    Texture2D,
    Cubemap,
    //Volume
  };

  struct ChannelMapping
  {
    ezInt8 m_iInput = 0;
    ezUInt8 m_uiChannelMask = 0;
  };

  TextureType m_TextureType;
  ezHybridArray<ezString, 4> m_InputFileNames;
  ezDynamicArray<ezImage> m_InputImages;
  ezString m_sOutputFile;
  bool m_bGeneratedMipmaps;
  bool m_bCompress;
  bool m_bSRGBOutput;
  ezUInt8 m_uiOutputChannels;
  ezHybridArray<ezImage*, 6> m_CleanupImages;
  ezUInt64 m_uiAssetHash;
  ezUInt16 m_uiAssetVersion;

  ChannelMapping m_2dSource[4];

  ezTexConv();

  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;

  ChannelMapping ParseInputCfg(const char* cfg, ezInt8 iChannelIndex, bool bSingleChannel);
  void ParseCommandLine();
  ezResult ValidateConfiguration();
  void PrintConfig();

  ezString ChannelMaskToString(ezUInt8 mask);

  ezResult LoadSingleInputFile(const char* szFile);
  ezResult LoadInputs();
  ezResult ConvertInputsToRGBA();

  float GetChannelValue(const ChannelMapping& ds, const ezUInt32 rgba);
  bool IsImageAlphaBinaryMask(const ezImage& img);

  ezImage* CreateCombinedFile(const ChannelMapping* dataSources);
  ezImageFormat::Enum ChooseOutputFormat(bool bSRGB, bool bAlphaIsMask) const;
  bool CanPassThroughInput() const;
  void WriteTexHeader(ezFileWriter& fileOut);

  virtual ezApplication::ApplicationExecution Run() override;
};
