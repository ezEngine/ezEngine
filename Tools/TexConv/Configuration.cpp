#include "Main.h"

#ifdef EZ_PLATFORM_WINDOWS
#include <d3d11.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <wrl\client.h>
#endif

// Only use GPU compression on windows as d3d11 is not available on any other platform
#ifdef EZ_PLATFORM_WINDOWS
namespace
{
  bool GetDXGIFactory(IDXGIFactory1** pFactory)
  {
    if (!pFactory)
      return false;

    *pFactory = nullptr;

    typedef HRESULT(WINAPI* pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void **ppFactory);

    static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

    if (!s_CreateDXGIFactory1)
    {
      HMODULE hModDXGI = LoadLibraryA("dxgi.dll");
      if (!hModDXGI)
        return false;

      s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));
      if (!s_CreateDXGIFactory1)
        return false;
    }

    return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
  }

  bool CreateDevice(int adapter, ID3D11Device** pDevice)
  {
    using namespace Microsoft::WRL;

    if (!pDevice)
      return false;

    *pDevice = nullptr;

    static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;

    if (!s_DynamicD3D11CreateDevice)
    {
      HMODULE hModD3D11 = LoadLibraryA("d3d11.dll");
      if (!hModD3D11)
        return false;

      s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
      if (!s_DynamicD3D11CreateDevice)
        return false;
    }

    D3D_FEATURE_LEVEL featureLevels[] =
    {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
    };

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<IDXGIAdapter> pAdapter;
    if (adapter >= 0)
    {
      ComPtr<IDXGIFactory1> dxgiFactory;
      if (GetDXGIFactory(dxgiFactory.GetAddressOf()))
      {
        if (FAILED(dxgiFactory->EnumAdapters(adapter, pAdapter.GetAddressOf())))
        {
          ezLog::Error("ERROR: Invalid GPU adapter index ({0})!\n", adapter);
          return false;
        }
      }
    }

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = s_DynamicD3D11CreateDevice(pAdapter.Get(),
      (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
      nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
      D3D11_SDK_VERSION, pDevice, &fl, nullptr);
    if (SUCCEEDED(hr))
    {
      if (fl < D3D_FEATURE_LEVEL_11_0)
      {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
        hr = (*pDevice)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
        if (FAILED(hr))
          memset(&hwopts, 0, sizeof(hwopts));

        if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
        {
          if (*pDevice)
          {
            (*pDevice)->Release();
            *pDevice = nullptr;
          }
          hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
      }
    }

    if (SUCCEEDED(hr))
    {
      ComPtr<IDXGIDevice> dxgiDevice;
      hr = (*pDevice)->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));
      if (SUCCEEDED(hr))
      {
        hr = dxgiDevice->GetAdapter(pAdapter.ReleaseAndGetAddressOf());
        if (SUCCEEDED(hr))
        {
          DXGI_ADAPTER_DESC desc;
          hr = pAdapter->GetDesc(&desc);
          if (SUCCEEDED(hr))
          {
            ezStringUtf8 sDesc(desc.Description);
            ezLog::Info("Using DirectCompute on \"{0}\"", sDesc.GetData());
          }
        }
      }

      return true;
    }
    else
      return false;
  }
}
#endif
ezTexConv::ezTexConv()
{
  m_TextureType = TextureType::Texture2D;
  m_bGeneratedMipmaps = false;
  m_bCompress = false;
  m_uiOutputChannels = 4;
  m_bSRGBOutput = false;
  m_bHDROutput = false;
  m_bPremultiplyAlpha = false;
  m_uiAddressU = 0;
  m_uiAddressV = 0;
  m_uiAddressW = 0;
  m_uiAssetHash = 0;
  m_uiAssetVersion = 0;
  m_bAlphaIsMaskOnly = false;
  m_pD3dDevice = nullptr;
}

void ezTexConv::AfterCoreStartup()
{
  CoInitialize(nullptr);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Add standard folder factory
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  m_2dSource[0].m_uiChannelMask = EZ_BIT(0);
  m_2dSource[1].m_uiChannelMask = EZ_BIT(1);
  m_2dSource[2].m_uiChannelMask = EZ_BIT(2);
  m_2dSource[3].m_uiChannelMask = EZ_BIT(3);

  #ifdef EZ_PLATFORM_WINDOWS
  CreateDevice(0, &m_pD3dDevice);
  #endif

  ParseCommandLine();
}

void ezTexConv::BeforeCoreShutdown()
{
  if (m_pD3dDevice)
  {
    m_pD3dDevice->Release();
    m_pD3dDevice = nullptr;
  }
  m_FileOut.Close();

  for (auto pImg : m_CleanupImages)
  {
    EZ_DEFAULT_DELETE(pImg);
  }

  m_CleanupImages.Clear();

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

ezTexConv::ChannelMapping ezTexConv::ParseInputCfg(const char* cfg, ezInt8 iChannelIndex, bool bSingleChannel)
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
      ezLog::Error("Could not parse input config '{0}'", cfg);
      return source;
    }

    // valid index after the 'in'
    if (num >= 0 && num < (ezInt32)m_InputFileNames.GetCount())
    {
      source.m_iInput = num;
    }
    else
    {
      ezLog::Error("Invalid Input index '{0}'", num);
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
    ezLog::Error("Expected '.' after input index in '{0}'", cfg);
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
    ezLog::Error("Bad input config '{0}'", cfg);

    source.m_uiChannelMask = EZ_BIT(iChannelIndex);
    return source;
  }

  // for multi-channel assignments do this exactly once
  // for single-channel assignments, find all channels that should be merged into it
  do
  {
    if (tmp.StartsWith_NoCase("r"))
      source.m_uiChannelMask |= Channel::Red;

    if (tmp.StartsWith_NoCase("g"))
      source.m_uiChannelMask |= Channel::Green;

    if (tmp.StartsWith_NoCase("b"))
      source.m_uiChannelMask |= Channel::Blue;

    if (tmp.StartsWith_NoCase("a"))
      source.m_uiChannelMask |= Channel::Alpha;

    tmp.Shrink(1, 0);
  }
  while (bSingleChannel && !tmp.IsEmpty());

  return source;
}

void ezTexConv::ParseCommandLine()
{
  auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  ezStringBuilder res, tmp;

  for (ezUInt32 i = 0; i < 32; ++i)
  {
    tmp.Format("-in{0}", i);

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

  // Texture Type
  {
    if (pCmd->GetBoolOption("-cubemap"))
      m_TextureType = TextureType::Cubemap;
  }

  if (m_TextureType == TextureType::Cubemap)
  {
    // 0 = +X = Right
    // 1 = -X = Left
    // 2 = +Y = Top
    // 3 = -Y = Bottom
    // 4 = +Z = Front
    // 5 = -Z = Back

    if (m_InputFileNames.IsEmpty())
    {
      m_InputFileNames.SetCount(6);

      m_InputFileNames[0] = pCmd->GetStringOption("-right");
      m_InputFileNames[1] = pCmd->GetStringOption("-left");
      m_InputFileNames[2] = pCmd->GetStringOption("-top");
      m_InputFileNames[3] = pCmd->GetStringOption("-bottom");
      m_InputFileNames[4] = pCmd->GetStringOption("-front");
      m_InputFileNames[5] = pCmd->GetStringOption("-back");
    }

    if (m_InputFileNames.IsEmpty())
    {
      m_InputFileNames.SetCount(6);

      m_InputFileNames[0] = pCmd->GetStringOption("-px");
      m_InputFileNames[1] = pCmd->GetStringOption("-nx");
      m_InputFileNames[2] = pCmd->GetStringOption("-py");
      m_InputFileNames[3] = pCmd->GetStringOption("-ny");
      m_InputFileNames[4] = pCmd->GetStringOption("-pz");
      m_InputFileNames[5] = pCmd->GetStringOption("-nz");
    }
  }


  // target file
  {
    m_sOutputFile = pCmd->GetStringOption("-out");
  }

  // some other states
  {
    m_bGeneratedMipmaps = pCmd->GetBoolOption("-mipmaps", false);
    m_bCompress = pCmd->GetBoolOption("-compress", false);
    m_bSRGBOutput = pCmd->GetBoolOption("-srgb", false);
    m_bHDROutput = pCmd->GetBoolOption("-hdr", false);
    m_uiOutputChannels = pCmd->GetIntOption("-channels", 4);
    m_bPremultiplyAlpha = pCmd->GetBoolOption("-premulalpha", false);

    m_uiFilterSetting = pCmd->GetIntOption("-filter", 9); // ezTextureFilterSetting::DefaultQuality
    m_uiAddressU = pCmd->GetIntOption("-addressU", 0);
    m_uiAddressV = pCmd->GetIntOption("-addressV", 0);
    m_uiAddressW = pCmd->GetIntOption("-addressW", 0);
  }

  // Thumbnail
  {
    m_sThumbnailFile = pCmd->GetStringOption("-thumbnail");
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

ezResult ezTexConv::ValidateConfiguration()
{
  if (m_InputFileNames.IsEmpty())
  {
    SetReturnCode(TexConvReturnCodes::VALIDATION_FAILED);
    ezLog::Error("No input files are specified. Use the -in command for a single file, or -in0 ... -in31 for multiple input files");
    return EZ_FAILURE;
  }

  if (m_sOutputFile.IsEmpty())
  {
    SetReturnCode(TexConvReturnCodes::VALIDATION_FAILED);
    ezLog::Error("No output file is specified. Use the -out command for this");
    return EZ_FAILURE;
  }

  if (m_uiOutputChannels > 4)
  {
    SetReturnCode(TexConvReturnCodes::VALIDATION_FAILED);
    ezLog::Error("Number of target channels ({0}) is invalid", m_uiOutputChannels);
    return EZ_FAILURE;
  }

  if (m_bHDROutput && m_bSRGBOutput)
  {
    SetReturnCode(TexConvReturnCodes::VALIDATION_FAILED);
    ezLog::Error("-srgb and -hdr falg can not be specified together as srgb textures can not be hdr.");
    return EZ_FAILURE;
  }

  if (m_uiOutputChannels < 3 && m_bSRGBOutput)
  {
    SetReturnCode(TexConvReturnCodes::VALIDATION_FAILED);
    ezLog::Error("-srgb flag is invalid for 1 and 2 channel textures, as they do not support sRGB output.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTexConv::PrintConfig()
{
  EZ_LOG_BLOCK("Configuration");

  ezLog::Info("Output: '{0}'", m_sOutputFile.GetData());

  for (ezUInt32 i = 0; i < m_InputFileNames.GetCount(); ++i)
  {
    ezLog::Info("Input {0}: '{1}'", i, m_InputFileNames[i].GetData());
  }

  ezLog::Info("Generate Mipmaps: {0}", m_bGeneratedMipmaps ? "yes" : "no");
  ezLog::Info("Use Compression: {0}", m_bCompress ? "yes" : "no");
  ezLog::Info("Output Channels: {0}", m_uiOutputChannels);
  ezLog::Info("Output is {0}", m_bSRGBOutput ? "sRGB" : "Linear");
  ezLog::Info("HDR Output: {0}", m_bHDROutput ? "yes" : "no");
  ezLog::Info("Pre-multiply alpha: {0}", m_bPremultiplyAlpha ? "yes" : "no");

  for (ezUInt32 i = 0; i < m_uiOutputChannels; ++i)
  {
    if (m_2dSource[i].m_iInput == -1)
    {
      ezLog::Info("Output[{0}] = {1}", i, m_2dSource[i].m_uiChannelMask == 0 ? "black" : "white");
    }
    else
    {
      ezLog::Info("Output[{0}] = Input[{1}].{2}", i, m_2dSource[i].m_iInput, ChannelMaskToString(m_2dSource[i].m_uiChannelMask).GetData());
    }
  }

  switch (m_TextureType)
  {
  case TextureType::Texture2D:
    ezLog::Info("Type: 2D Texture");
    break;
  case TextureType::Cubemap:
    ezLog::Info("Type: Cubemap");
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}
