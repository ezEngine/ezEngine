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

/// \todo volume texture creation
/// \todo resizing or downscaling to closest POT or max resolution
/// \todo normalmap generation from heightmaps
/// \todo Normalmap flag (mipmaps?)
/// \todo sRGB auto detection

/// \todo Write thumbnail to additional location
/// \todo Pass Through original when no change needed
/// \todo Reading (compressed) TGA very slow
/// \todo cubemap creation


/**** Usage ****

-out "file" -> defines where to write the output to
-in "file" -> same as -in0 "file"
-in0 "file" -> defines input file 0
-in31 "file" -> defines input file 31
-mipmap -> enables mipmap generation
-srgb -> the output format will be an SRGB format, otherwise linear, cannot be used for 1 and 2 channel formats
-compress -> the output format will be a compressed format
-channels X -> the output format will have 1, 2, 3 or 4 channels, default is 4
-r inX.r -> the output RED channel is taken from the RED channel of input file X. The input channel is considered to be in Gamma space!
-g inX.x -> the output GREEN channel is taken from the RED channel of input file X. The input channel is considered to be in Linear space!
-b inX.rgb -> the output BLUE channel is the weighted average of the RGB channels in input file X. The RGB channels are considered to be in Gamma space!
-rgba inX.rrra -> the output RGB cannels are all initialized from the RED channel of input X. Alpha is copied directly.

For the output you can use
-r, -g, -b, -a, -rg, -rgb, -rgba

For input you can use
r,g,b,a for gamma space values (alpha is always linear, though)
x,y,z,w for linear space values
and any combination of rgb, bgar, xyz, rgxy, etc. for swizzling

When a single channel is written (e.g. -r) you can read from multiple channels (e.g. rgb) to create an averaged value.

*/

using namespace DirectX;

class ezTexConv : public ezApplication
{
public:

  enum Channel
  {
    None = 0,
    RedSRGB = EZ_BIT(0),
    GreenSRGB = EZ_BIT(1),
    BlueSRGB = EZ_BIT(2),
    AlphaSRGB = EZ_BIT(3),
    RedLinear = EZ_BIT(4),
    GreenLinear = EZ_BIT(5),
    BlueLinear = EZ_BIT(6),
    AlphaLinear = EZ_BIT(7),
    All = 0xFF
  };

  struct ChannelMapping
  {
    ezInt8 m_iInput = 0;
    ezUInt8 m_uiChannelMask = 0;
  };

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

  ezTexConv()
  {
    m_bGeneratedMipmaps = false;
    m_bCompress = false;
    m_uiOutputChannels = 4;
    m_bSRGBOutput = false;
    m_uiAssetHash = 0;
    m_uiAssetVersion = 0;
  }

  virtual void AfterCoreStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("");

    m_2dSource[0].m_uiChannelMask = EZ_BIT(0);
    m_2dSource[1].m_uiChannelMask = EZ_BIT(1);
    m_2dSource[2].m_uiChannelMask = EZ_BIT(2);
    m_2dSource[3].m_uiChannelMask = EZ_BIT(3);

    ParseCommandLine();
  }

  virtual void BeforeCoreShutdown()
  {
    for (auto pImg : m_CleanupImages)
    {
      EZ_DEFAULT_DELETE(pImg);
    }

    m_CleanupImages.Clear();

    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  ChannelMapping ParseInputCfg(const char* cfg, ezInt8 iChannelIndex, bool bSingleChannel)
  {
    ChannelMapping source;
    source.m_iInput = -1;
    source.m_uiChannelMask = 0;

    ezStringBuilder tmp = cfg;

    // '-r black' for setting it to zero
    if (tmp.IsEqual_NoCase("black"))
      return source;

    // 'r white' for setting it to 255
    if (tmp.IsEqual_NoCase("white"))
    {
      source.m_uiChannelMask = Channel::All;
      return source;
    }

    // skip the 'in', if found
    // is optional though, one can also write '-r 1.r' for '-r in1.r'
    if (tmp.StartsWith_NoCase("in"))
      tmp.Shrink(2, 0);

    if (tmp.StartsWith("."))
    {
      // no index given, e.g. '-r in.r'
      // in is equal to in0

      source.m_iInput = 0;
    }
    else
    {
      ezInt32 num = -1;
      const char* szLastPos = nullptr;
      if (ezConversionUtils::StringToInt(tmp, num, &szLastPos).Failed())
      {
        ezLog::Error("Could not parse input config '%s'", cfg);
        return source;
      }

      // valid index after the 'in'
      if (num >= 0 && num < (ezInt32)m_InputFileNames.GetCount())
      {
        source.m_iInput = num;
      }
      else
      {
        ezLog::Error("Invalid Input index '%u'", num);
        return source;
      }

      ezStringBuilder dummy = szLastPos;

      // continue after the index
      tmp = dummy;
    }

    // no additional info, e.g. '-g in2' is identical to '-g in2.g' (same channel)
    if (tmp.IsEmpty())
    {
      source.m_uiChannelMask = EZ_BIT(iChannelIndex);
      return source;
    }

    if (!tmp.StartsWith("."))
    {
      ezLog::Error("Expected '.' after input index in '%s'", cfg);
      return source;
    }

    tmp.Shrink(1, 0);

    if (!bSingleChannel)
    {
      // in case of '-rgb in1.bgr' map r to b, g to g, b to r, etc.
      // in case of '-rgb in1.r' map everything to the same input
      if (tmp.GetCharacterCount() > 1)
        tmp.Shrink(iChannelIndex, 0);
    }

    // no additional info, e.g. '-rgb in2.rg' will map b to b
    if (tmp.IsEmpty())
    {
      ezLog::Error("Bad input config '%s'", cfg);

      source.m_uiChannelMask = EZ_BIT(iChannelIndex);
      return source;
    }

    // for multi-channel assignments do this exactly once
    // for single-channel assignments, find all channels that should be merged into it
    do
    {
      // sRGB input
      {
        if (tmp.StartsWith_NoCase("r"))
          source.m_uiChannelMask |= Channel::RedSRGB;

        if (tmp.StartsWith_NoCase("g"))
          source.m_uiChannelMask |= Channel::GreenSRGB;

        if (tmp.StartsWith_NoCase("b"))
          source.m_uiChannelMask |= Channel::BlueSRGB;

        if (tmp.StartsWith_NoCase("a"))
          source.m_uiChannelMask |= Channel::AlphaSRGB;
      }

      // linear input
      {
        if (tmp.StartsWith_NoCase("x"))
          source.m_uiChannelMask |= Channel::RedLinear;

        if (tmp.StartsWith_NoCase("y"))
          source.m_uiChannelMask |= Channel::GreenLinear;

        if (tmp.StartsWith_NoCase("z"))
          source.m_uiChannelMask |= Channel::BlueLinear;

        if (tmp.StartsWith_NoCase("w"))
          source.m_uiChannelMask |= Channel::AlphaLinear;
      }

      tmp.Shrink(1, 0);
    }
    while (bSingleChannel && !tmp.IsEmpty());

    return source;
  }

  void ParseCommandLine()
  {
    auto pCmd = ezCommandLineUtils::GetGlobalInstance();

    ezStringBuilder res, tmp;

    for (ezUInt32 i = 0; i < 32; ++i)
    {
      tmp.Format("-in%u", i);

      res = pCmd->GetStringOption(tmp);

      // stop once one option was not found
      if (res.IsEmpty())
        break;

      m_InputFileNames.PushBack(res);
    }

    if (m_InputFileNames.IsEmpty()) // try '-in' in this case, ignore it otherwise
    {
      // short version for -in1
      res = pCmd->GetStringOption("-in");

      if (!res.IsEmpty())
        m_InputFileNames.PushBack(res);
    }

    // target file
    {
      m_sOutputFile = pCmd->GetStringOption("-out");
    }

    // some boolean states
    {
      m_bGeneratedMipmaps = pCmd->GetBoolOption("-mipmaps", false);
      m_bCompress = pCmd->GetBoolOption("-compress", false);
      m_bSRGBOutput = pCmd->GetBoolOption("-srgb", false);
      m_uiOutputChannels = pCmd->GetIntOption("-channels", 4);
    }

    // input to output mappings
    {
      tmp = pCmd->GetStringOption("-rgba");
      if (!tmp.IsEmpty())
      {
        m_2dSource[0] = ParseInputCfg(tmp, 0, false);
        m_2dSource[1] = ParseInputCfg(tmp, 1, false);
        m_2dSource[2] = ParseInputCfg(tmp, 2, false);
        m_2dSource[3] = ParseInputCfg(tmp, 3, false);
      }

      tmp = pCmd->GetStringOption("-rgb");
      if (!tmp.IsEmpty())
      {
        m_2dSource[0] = ParseInputCfg(tmp, 0, false);
        m_2dSource[1] = ParseInputCfg(tmp, 1, false);
        m_2dSource[2] = ParseInputCfg(tmp, 2, false);
      }

      tmp = pCmd->GetStringOption("-rg");
      if (!tmp.IsEmpty())
      {
        m_2dSource[0] = ParseInputCfg(tmp, 0, false);
        m_2dSource[1] = ParseInputCfg(tmp, 1, false);
      }

      tmp = pCmd->GetStringOption("-r");
      if (!tmp.IsEmpty())
        m_2dSource[0] = ParseInputCfg(tmp, 0, true);

      tmp = pCmd->GetStringOption("-g");
      if (!tmp.IsEmpty())
        m_2dSource[1] = ParseInputCfg(tmp, 1, true);

      tmp = pCmd->GetStringOption("-b");
      if (!tmp.IsEmpty())
        m_2dSource[2] = ParseInputCfg(tmp, 2, true);

      tmp = pCmd->GetStringOption("-a");
      if (!tmp.IsEmpty())
        m_2dSource[3] = ParseInputCfg(tmp, 3, true);
    }

    // Asset Hash and Version
    {
      m_uiAssetVersion = pCmd->GetIntOption("-assetVersion", 0);

      const ezString sHashLow = pCmd->GetStringOption("-assetHashLow");
      ezUInt64 uiHashLow = ezConversionUtils::ConvertHexStringToUInt32(sHashLow);
      const ezString sHashHigh = pCmd->GetStringOption("-assetHashHigh");
      ezUInt64 uiHashHigh = ezConversionUtils::ConvertHexStringToUInt32(sHashHigh);

      m_uiAssetHash = (uiHashHigh << 32) | uiHashLow;
    }
  }

  ezResult ValidateConfiguration()
  {
    if (m_InputFileNames.IsEmpty())
    {
      ezLog::Error("No input files are specified. Use the -in command for a single file, or -in0 ... -in31 for multiple input files");
      return EZ_FAILURE;
    }

    if (m_sOutputFile.IsEmpty())
    {
      ezLog::Error("No output file is specified. Use the -out command for this");
      return EZ_FAILURE;
    }

    if (m_uiOutputChannels > 4)
    {
      ezLog::Error("Number of target channels (%u) is invalid", m_uiOutputChannels);
      return EZ_FAILURE;
    }

    if (m_uiOutputChannels < 3 && m_bSRGBOutput)
    {
      ezLog::Error("-srgb flag is invalid for 1 and 2 channel textures, as they do not support sRGB output.");
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezString ChannelMaskToString(ezUInt8 mask)
  {
    ezStringBuilder s("<");

    if ((mask & Channel::RedSRGB) != 0)
      s.Append("r");
    if ((mask & Channel::GreenSRGB) != 0)
      s.Append("g");
    if ((mask & Channel::BlueSRGB) != 0)
      s.Append("b");
    if ((mask & Channel::AlphaSRGB) != 0)
      s.Append("a");
    if ((mask & Channel::RedLinear) != 0)
      s.Append("x");
    if ((mask & Channel::GreenLinear) != 0)
      s.Append("y");
    if ((mask & Channel::BlueLinear) != 0)
      s.Append("z");
    if ((mask & Channel::AlphaLinear) != 0)
      s.Append("w");

    s.Append(">");

    return s;
  }

  void PrintConfig()
  {
    EZ_LOG_BLOCK("Configuration");

    ezLog::Info("Output: '%s'", m_sOutputFile.GetData());

    for (ezUInt32 i = 0; i < m_InputFileNames.GetCount(); ++i)
    {
      ezLog::Info("Input %u: '%s'", i, m_InputFileNames[i].GetData());
    }

    ezLog::Info("Generate Mipmaps: %s", m_bGeneratedMipmaps ? "yes" : "no");
    ezLog::Info("Use Compression: %s", m_bCompress ? "yes" : "no");
    ezLog::Info("Output Channels: %u", m_uiOutputChannels);

    for (ezUInt32 i = 0; i < m_uiOutputChannels; ++i)
    {
      if (m_2dSource[i].m_iInput == -1)
      {
        ezLog::Info("Output[%u] = %s", i, m_2dSource[i].m_uiChannelMask == 0 ? "black" : "white");
      }
      else
      {
        ezLog::Info("Output[%u] = Input[%i].%s", i, m_2dSource[i].m_iInput, ChannelMaskToString(m_2dSource[i].m_uiChannelMask).GetData());
      }
    }
  }

  ezResult LoadSingleInputFile(const char* szFile)
  {
    ezImage& source = m_InputImages.ExpandAndGetRef();
    if (source.LoadFrom(szFile).Failed())
    {
      ezLog::Error("Failed to load file '%s'", szFile);
      return EZ_FAILURE;
    }

    if (ezImageConversion::Convert(source, source, ezImageFormat::R8G8B8A8_UNORM).Failed())
    {
      ezLog::Error("Failed to convert file '%s' from format %s to R8G8B8A8_UNORM. Format is not supported.", ezImageFormat::GetName(source.GetImageFormat()), szFile);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezResult LoadInputs()
  {
    EZ_LOG_BLOCK("Load Inputs");

    m_InputImages.Reserve(m_InputFileNames.GetCount());

    for (const auto& in : m_InputFileNames)
    {
      if (LoadSingleInputFile(in).Failed())
        return EZ_FAILURE;
    }

    for (ezUInt32 i = 1; i < m_InputImages.GetCount(); ++i)
    {
      if (m_InputImages[i].GetWidth() != m_InputImages[0].GetWidth() ||
          m_InputImages[i].GetHeight() != m_InputImages[0].GetHeight())
      {
        ezLog::Error("Input image %u has a different resolution than image 0. This is currently not supported.", i);
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  static inline float GetChannelValueSRGB(ezUInt32 rawColor, float& numSources)
  {
    float fRaw = ezMath::ColorByteToFloat(rawColor);
    fRaw = ezColor::GammaToLinear(fRaw);
    numSources += 1.0f;
    return fRaw;
  }

  static inline float GetChannelValueLinear(ezUInt32 rawColor, float& numSources)
  {
    float fRaw = ezMath::ColorByteToFloat(rawColor);
    numSources += 1.0f;
    return fRaw;
  }

  float ezTexConv::GetChannelValue(const ChannelMapping& ds, const ezUInt32 rgba)
  {
    float numSources = 0.0f;
    float channelValue = 0.0f;

    if ((ds.m_uiChannelMask & Channel::RedSRGB) != 0)
      channelValue += GetChannelValueSRGB((rgba & 0x000000FF), numSources);

    if ((ds.m_uiChannelMask & Channel::GreenSRGB) != 0)
      channelValue += GetChannelValueSRGB((rgba & 0x0000FF00) >> 8, numSources);

    if ((ds.m_uiChannelMask & Channel::BlueSRGB) != 0)
      channelValue += GetChannelValueSRGB((rgba & 0x00FF0000) >> 16, numSources);

    if ((ds.m_uiChannelMask & Channel::AlphaSRGB) != 0)
      channelValue += GetChannelValueSRGB((rgba & 0xFF000000) >> 24, numSources);

    if ((ds.m_uiChannelMask & Channel::RedLinear) != 0)
      channelValue += GetChannelValueLinear((rgba & 0x000000FF), numSources);

    if ((ds.m_uiChannelMask & Channel::GreenLinear) != 0)
      channelValue += GetChannelValueLinear((rgba & 0x0000FF00) >> 8, numSources);

    if ((ds.m_uiChannelMask & Channel::BlueLinear) != 0)
      channelValue += GetChannelValueLinear((rgba & 0x00FF0000) >> 16, numSources);

    if ((ds.m_uiChannelMask & Channel::AlphaLinear) != 0)
      channelValue += GetChannelValueLinear((rgba & 0xFF000000) >> 24, numSources);

    if (numSources > 0.0f)
      channelValue /= numSources;

    return channelValue;
  }

  ezImage* CreateCombinedFile(const ChannelMapping* dataSources)
  {
    /// \todo Handle different input sizes

    ezImage* pImg = EZ_DEFAULT_NEW(ezImage);
    m_CleanupImages.PushBack(pImg);

    /// \todo Return loaded image pointer, if no combination is necessary

    const ezUInt32 uiWidth = m_InputImages[0].GetWidth();
    const ezUInt32 uiHeight = m_InputImages[0].GetHeight();

    pImg->SetWidth(uiWidth);
    pImg->SetHeight(uiHeight);
    pImg->SetDepth(1);
    pImg->SetNumArrayIndices(1);
    pImg->SetNumFaces(1);
    pImg->SetNumMipLevels(1);
    pImg->SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    pImg->AllocateImageData();

    // later block compression may pre-multiply rgb by alpha, if we have never set alpha to anything (3 channel case), that will result in black
    ezUInt32 uiDefaultResult = 0;
    if (m_uiOutputChannels < 4)
      uiDefaultResult = 0xFF000000;

    // not the most efficient loops...
    for (ezUInt32 h = 0; h < uiHeight; ++h)
    {
      for (ezUInt32 w = 0; w < uiWidth; ++w)
      {
        ezUInt32 uiResultRGBA = uiDefaultResult;

        for (ezUInt32 i = 0; i < m_uiOutputChannels; ++i)
        {
          const auto& ds = dataSources[i];

          float channelValue = 0.0f;

          if (ds.m_iInput == -1)
          {
            // handles 'black' and 'white' values

            if (ds.m_uiChannelMask == Channel::All)
              channelValue = 1.0f;
          }
          else
          {
            const ezImage& src = m_InputImages[ds.m_iInput];
            const ezUInt32 rgba = *src.GetPixelPointer<ezUInt32>(0, 0, 0, w, h, 0);

            channelValue = GetChannelValue(ds, rgba);
          }

          // we build all images in linear space and set the SRGB format at the end (or not)

          const ezUInt32 uiFinalUB = ezMath::ColorFloatToByte(channelValue);

          uiResultRGBA |= uiFinalUB << (i * 8);
        }

        *pImg->GetPixelPointer<ezUInt32>(0, 0, 0, w, h, 0) = uiResultRGBA;
      }
    }

    return pImg;
  }

  ezImageFormat::Enum ChooseOutputFormat() const
  {
    if (m_bCompress)
    {
      if (m_uiOutputChannels == 1)
        return ezImageFormat::BC4_UNORM;

      if (m_uiOutputChannels == 2)
        return ezImageFormat::BC5_UNORM;

      if (m_uiOutputChannels == 3)
        return m_bSRGBOutput ? ezImageFormat::BC1_UNORM_SRGB : ezImageFormat::BC1_UNORM;

      if (m_uiOutputChannels == 4)
      {
        /// \todo Use BC1 if entire alpha channel is either 0 or 255 (mask)
        return m_bSRGBOutput ? ezImageFormat::BC3_UNORM_SRGB : ezImageFormat::BC3_UNORM;
      }
    }
    else
    {
      if (m_uiOutputChannels == 1)
        return ezImageFormat::R8_UNORM;

      if (m_uiOutputChannels == 2)
        return ezImageFormat::R8G8_UNORM;

      if (m_uiOutputChannels == 3)
      {
        /// \todo Use B8G8R8X8 ?? I think it is not properly supported everywhere
        return m_bSRGBOutput ? ezImageFormat::R8G8B8A8_UNORM_SRGB : ezImageFormat::R8G8B8A8_UNORM;
      }

      if (m_uiOutputChannels == 4)
      {
        return m_bSRGBOutput ? ezImageFormat::R8G8B8A8_UNORM_SRGB : ezImageFormat::R8G8B8A8_UNORM;
      }
    }

    EZ_REPORT_FAILURE("ChooseOutputFormat: Invalid control flow");
    return ezImageFormat::R8G8B8A8_UNORM_SRGB;
  }


  virtual ezApplication::ApplicationExecution Run() override
  {
    // general failure
    SetReturnCode(1);

    PrintConfig();

    if (ValidateConfiguration().Failed())
    {
      SetReturnCode(2);
      return ezApplication::Quit;
    }

    ezFileWriter fileOut;
    if (fileOut.Open(m_sOutputFile, 1024 * 1024 * 8).Failed())
    {
      SetReturnCode(3);
      ezLog::Error("Could not open output file for writing: '%s'", m_sOutputFile.GetData());
      return ezApplication::Quit;
    }

    if (LoadInputs().Failed())
    {
      SetReturnCode(4);
      return ezApplication::Quit;
    }

    ezImage* pCombined = CreateCombinedFile(m_2dSource);

    CoInitialize(nullptr);

    Image srcImg;
    srcImg.width = pCombined->GetWidth();
    srcImg.height = pCombined->GetHeight();
    srcImg.rowPitch = pCombined->GetRowPitch();
    srcImg.slicePitch = pCombined->GetDepthPitch();
    srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(pCombined->GetImageFormat());
    srcImg.pixels = pCombined->GetDataPointer<ezUInt8>();

    ScratchImage mip, comp, channel;
    ScratchImage* pCurScratch = nullptr;

    if (m_bGeneratedMipmaps)
    {
      if (FAILED(GenerateMipMaps(srcImg, TEX_FILTER_DEFAULT, 0, mip)))
      {
        SetReturnCode(5);
        ezLog::Error("Mipmap generation failed");
        return ezApplication::Quit;
      }

      pCurScratch = &mip;
    }

    const ezImageFormat::Enum outputFormat = ChooseOutputFormat();
    const DXGI_FORMAT dxgi = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(outputFormat);

    Blob outputBlob;

    if (m_bCompress)
    {
      if (pCurScratch != nullptr)
      {
        if (FAILED(Compress(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), dxgi, TEX_COMPRESS_DEFAULT, 1.0f, comp)))
        {
          SetReturnCode(6);
          ezLog::Error("Block compression failed");
          return ezApplication::Quit;
        }
      }
      else
      {
        if (FAILED(Compress(srcImg, dxgi, TEX_COMPRESS_DEFAULT, 1.0f, comp)))
        {
          SetReturnCode(6);
          ezLog::Error("Block compression failed");
          return ezApplication::Quit;
        }
      }

      if (FAILED(SaveToDDSMemory(comp.GetImages(), comp.GetImageCount(), comp.GetMetadata(), 0, outputBlob)))
      {
        SetReturnCode(7);
        ezLog::Error("Failed to write compressed image to file '%s'", m_sOutputFile.GetData());
        return ezApplication::Quit;
      }
    }
    else
    {
      if (outputFormat != pCombined->GetImageFormat())
      {
        if (pCurScratch != nullptr)
        {
          if (FAILED(Convert(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), dxgi, TEX_FILTER_DEFAULT, 0.0f, channel)))
          {
            SetReturnCode(8);
            ezLog::Error("Failed to convert uncompressed image to %u channels", m_uiOutputChannels);
            return ezApplication::Quit;
          }
        }
        else
        {
          if (FAILED(Convert(srcImg, dxgi, TEX_FILTER_DEFAULT, 0.0f, channel)))
          {
            SetReturnCode(8);
            ezLog::Error("Failed to convert uncompressed image to %u channels", m_uiOutputChannels);
            return ezApplication::Quit;
          }
        }

        pCurScratch = &channel;
      }

      if (pCurScratch != nullptr)
      {
        if (FAILED(SaveToDDSMemory(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), 0, outputBlob)))
        {
          SetReturnCode(9);
          ezLog::Error("Failed to write uncompressed image to file '%s'", m_sOutputFile.GetData());
          return ezApplication::Quit;
        }
      }
      else
      {
        if (FAILED(SaveToDDSMemory(srcImg, 0, outputBlob)))
        {
          SetReturnCode(9);
          ezLog::Error("Failed to write uncompressed image to file '%s'", m_sOutputFile.GetData());
          return ezApplication::Quit;
        }
      }
    }

    if (ezPathUtils::HasExtension(m_sOutputFile, "ezTex"))
    {
      ezAssetFileHeader header;
      header.SetFileHashAndVersion(m_uiAssetHash, m_uiAssetVersion);

      header.Write(fileOut);

      fileOut << m_bSRGBOutput;
    }

    fileOut.WriteBytes(outputBlob.GetBufferPointer(), outputBlob.GetBufferSize());

    // everything is fine
    SetReturnCode(0);
    return ezApplication::Quit;
  }
};



EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv);
