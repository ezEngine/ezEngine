#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Strings/StringConversion.h>
#include <Texture/Image/Conversions/DXTConversions.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/ImageConversion.h>

ezCVarBool cvar_TexturePenalizeDXConversions("Texture.PenalizeDXConversions", false, ezCVarFlags::RequiresRestart, "Add a penalty to DirectX-based conversion when choosing how to convert textures");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  define EZ_SUPPORTS_DIRECTXTEX EZ_ON
#else
#  define EZ_SUPPORTS_DIRECTXTEX EZ_OFF
#endif

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTXTEX)

#  ifdef DeleteFile
#    undef DeleteFile
#  endif

#  include <Texture/DirectXTex/DirectXTex.h>
#  include <d3d11.h>
#  include <dxgi.h>
#  include <dxgi1_6.h>
#  include <dxgiformat.h>
#  include <wrl\client.h>

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{
  bool GetDXGIFactory(IDXGIFactory1** pFactory)
  {
    if (!pFactory)
      return false;

    *pFactory = nullptr;

    using pfn_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID riid, _Out_ void** ppFactory);

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

  ComPtr<IDXGIAdapter1> CreateHighPerformanceAdapter()
  {
    ComPtr<IDXGIAdapter1> pAdapter;
    ComPtr<IDXGIFactory1> pFactory1;
    ComPtr<IDXGIFactory6> pFactory6;
    if (!GetDXGIFactory(pFactory1.GetAddressOf()))
      return {};

    if (FAILED(pFactory1->QueryInterface(IID_PPV_ARGS(pFactory6.GetAddressOf()))))
      return {};

    if (pFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(pAdapter.ReleaseAndGetAddressOf())) == DXGI_ERROR_NOT_FOUND)
      return {};

    return pAdapter;
  }

  bool TryCreateD3D11Device(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE driverType, ComPtr<ID3D11Device>& ref_device)
  {
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

    static const D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL SelectedFeatureLevel;

    for (int featureLevelIdx = 0; featureLevelIdx < EZ_ARRAY_SIZE(FeatureLevels); ++featureLevelIdx)
    {
      if (SUCCEEDED(s_DynamicD3D11CreateDevice(pAdapter, driverType, nullptr, 0, &FeatureLevels[featureLevelIdx], 1, D3D11_SDK_VERSION, ref_device.ReleaseAndGetAddressOf(), &SelectedFeatureLevel, nullptr)))
      {
        return true;
      }
    }
    return false;
  }

  enum class TypeOfDeviceCreated
  {
    None,
    Hardware,
    Software
  };

  /// Tries to create a hardware device, but falls back to a software device if there is one.
  TypeOfDeviceCreated CreateDevice(ComPtr<ID3D11Device>& ref_device)
  {
    ref_device = nullptr;
    TypeOfDeviceCreated deviceType = TypeOfDeviceCreated::None;

    ComPtr<IDXGIAdapter1> pAdapter = CreateHighPerformanceAdapter();
    if (pAdapter != nullptr && TryCreateD3D11Device(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, ref_device))
    {
      deviceType = TypeOfDeviceCreated::Hardware;
    }

    // Fallback: try to create a hardware device without specifying an adapter
    if (ref_device == nullptr && TryCreateD3D11Device(nullptr, D3D_DRIVER_TYPE_HARDWARE, ref_device))
    {
      deviceType = TypeOfDeviceCreated::Hardware;
    }

    // Fallback: try to create a WARP software device
    if (ref_device == nullptr && TryCreateD3D11Device(nullptr, D3D_DRIVER_TYPE_WARP, ref_device))
    {
      deviceType = TypeOfDeviceCreated::Software;
    }

    if (ref_device == nullptr)
      return TypeOfDeviceCreated::None;

    auto GetDeviceName = [&]() -> ezString
    {
      ComPtr<IDXGIDevice> pDXGIDevice;
      if (FAILED(ref_device.As(&pDXGIDevice)))
        return "<No IDXGIDevice interface>"_ezsv;
      ComPtr<IDXGIAdapter> pAdapter;
      if (FAILED(pDXGIDevice->GetAdapter(pAdapter.GetAddressOf())))
        return "<GetAdapter Failed>"_ezsv;
      DXGI_ADAPTER_DESC desc;
      if (FAILED(pAdapter->GetDesc(&desc)))
        return "<GetDesc Failed>"_ezsv;
      ezStringUtf8 sDesc(desc.Description);
      return ezString(sDesc.GetData());
    };
    ezLog::Dev("Using DirectCompute on \"{0}\"", GetDeviceName());
    return deviceType;
  }

  /// Ensures a device and its corresponding conversion table are constructed together.
  class DeviceAndConversionTable
  {
  public:
    /// Provides locked access to the table and its DX device.
    class ScopedAccess
    {
    public:
      ScopedAccess(DeviceAndConversionTable& ref_table)
        : m_Lock(ref_table.m_Mutex)
        , m_Table(ref_table)
      {
        ref_table.Init();
      }

      DeviceAndConversionTable* operator->() { return &m_Table; }

    private:
      ezLock<ezMutex> m_Lock;
      DeviceAndConversionTable& m_Table;
    };

    /// Get temporary access to the static DeviceAndConversionTable.
    static ScopedAccess getDeviceAndConversionTable() { return s_DeviceAndConversionTable; }
    static bool IsDeviceAndConversionTableInitialized() { return s_bDeviceAndTableInitialized; }

    ID3D11Device* getDevice() { return m_D3dDevice.Get(); }

    ezArrayPtr<const ezImageConversionEntry> getConvertors() const { return m_SupportedConversions; }

    void Init()
    {
      s_bDeviceAndTableInitialized = true;

      if (!m_SupportedConversions.IsEmpty())
        return;

      // A high penalty for software devices.
      // The number chosen is greater than (32 * 4) * 2 * 2 * 2 = 1024, the estimated cost of the
      // currently largest step, making software DX conversions available but highly undesirable.
      float devicePenalty = 2000.0f;

      if ((CreateDevice(m_D3dDevice) == TypeOfDeviceCreated::Hardware) && !cvar_TexturePenalizeDXConversions)
      {
        // No penalty for hardware devices.
        devicePenalty = 0.0f;
      }

      for (auto& entry : ezArrayPtr<ezImageConversionEntry>(s_sourceConversions))
      {
        entry.m_fAdditionalPenalty = devicePenalty;
      }

      m_SupportedConversions = s_sourceConversions;
    }

    void Deinit()
    {
      m_D3dDevice = nullptr;
      m_SupportedConversions.Clear();
      s_bDeviceAndTableInitialized = false;
    }

  private:
    ComPtr<ID3D11Device> m_D3dDevice = nullptr;
    ezArrayPtr<const ezImageConversionEntry> m_SupportedConversions;

    constexpr static int s_numConversions = 5;
    static ezImageConversionEntry s_sourceConversions[s_numConversions];

    ezMutex m_Mutex;
    static bool s_bDeviceAndTableInitialized;
    static DeviceAndConversionTable s_DeviceAndConversionTable;
  };

  ezImageConversionEntry DeviceAndConversionTable::s_sourceConversions[s_numConversions] = {
    ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC6H_UF16, ezImageConversionFlags::Default),

    ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default),
    ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default),

    ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default),
    ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default),
  };

  bool DeviceAndConversionTable::s_bDeviceAndTableInitialized = false;
  DeviceAndConversionTable DeviceAndConversionTable::s_DeviceAndConversionTable;
} // namespace

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(TexConv, DXTexConversions)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (DeviceAndConversionTable::IsDeviceAndConversionTableInitialized())
    {
      DeviceAndConversionTable::getDeviceAndConversionTable()->Deinit();
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

class ezImageConversion_CompressDxTex : public ezImageConversionStepCompressBlocks
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    return DeviceAndConversionTable::getDeviceAndConversionTable()->getConvertors();
  }

  virtual ezResult CompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 uiNumBlocksX, ezUInt32 uiNumBlocksY,
    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    const ezUInt32 targetWidth = uiNumBlocksX * ezImageFormat::GetBlockWidth(targetFormat);
    const ezUInt32 targetHeight = uiNumBlocksY * ezImageFormat::GetBlockHeight(targetFormat);

    Image srcImg;
    srcImg.width = targetWidth;
    srcImg.height = targetHeight;
    srcImg.rowPitch = static_cast<size_t>(ezImageFormat::GetRowPitch(sourceFormat, targetWidth));
    srcImg.slicePitch = static_cast<size_t>(ezImageFormat::GetDepthPitch(sourceFormat, targetWidth, targetHeight));

    // We don't trust anyone to handle sRGB correctly, so pretend we always want to compress linear -> linear even when it's actually sRGB -> sRGB.
    srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(ezImageFormat::AsLinear(sourceFormat));
    srcImg.pixels = (uint8_t*)static_cast<const void*>(source.GetPtr());

    ScratchImage dxSrcImage;
    if (FAILED(dxSrcImage.InitializeFromImage(srcImg)))
      return EZ_FAILURE;

    const DXGI_FORMAT dxgiTargetFormat = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(ezImageFormat::AsLinear(targetFormat));

    ScratchImage dxDstImage;

    bool bCompressionDone = false;

    {
      auto deviceAndConversionTableScope = DeviceAndConversionTable::getDeviceAndConversionTable();
      ID3D11Device* pD3dDevice = deviceAndConversionTableScope->getDevice();
      if (pD3dDevice != nullptr)
      {
        if (SUCCEEDED(Compress(pD3dDevice, dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat,
              TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
        {
          // Not all formats can be compressed on the GPU. Fall back to CPU in case GPU compression fails.
          bCompressionDone = true;
        }
      }
    }

    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(
            dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat, TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }
    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(
            dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat, TEX_COMPRESS_DEFAULT, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }

    if (!bCompressionDone)
      return EZ_FAILURE;

    target.CopyFrom(ezConstByteBlobPtr(dxDstImage.GetPixels(), static_cast<ezUInt32>(dxDstImage.GetPixelsSize())));

    return EZ_SUCCESS;
  }
};

EZ_STATICLINK_FORCE static ezImageConversion_CompressDxTex s_conversion_compressDxTex;

#endif



EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexConversions);
