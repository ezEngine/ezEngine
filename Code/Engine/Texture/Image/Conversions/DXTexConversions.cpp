#include <TexturePCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Strings/StringConversion.h>
#include <Texture/Image/Conversions/DXTConversions.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/ImageConversion.h>
#include <memory>

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
#  include <dxgiformat.h>
#  include <wrl\client.h>

using namespace std;
using namespace DirectX;

namespace
{
  bool GetDXGIFactory(IDXGIFactory1** pFactory)
  {
    if (!pFactory)
      return false;

    *pFactory = nullptr;

    typedef HRESULT(WINAPI * pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void** ppFactory);

    static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

    if (!s_CreateDXGIFactory1)
    {
      HMODULE hModDXGI = LoadLibraryA("dxgi.dll");
      if (!hModDXGI)
        return false;

      s_CreateDXGIFactory1 =
        reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));
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

      s_DynamicD3D11CreateDevice =
        reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
      if (!s_DynamicD3D11CreateDevice)
        return false;
    }

    D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
    };

    UINT createDeviceFlags = 0;
#  ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#  endif

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
    HRESULT hr = s_DynamicD3D11CreateDevice(pAdapter.Get(), (pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr,
      createDeviceFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, pDevice, &fl, nullptr);
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
} // namespace



class ezImageConversion_CompressDxTex : public ezImageConversionStepCompressBlocks
{
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    static ezImageConversionEntry supportedConversions[] = {
      ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC6H_UF16, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default),

      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC6H_UF16, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default),

      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC6H_UF16, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default),
      ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual ezResult CompressBlocks(ezBlobPtr<const void> source, ezBlobPtr<void> target, ezUInt32 numBlocksX, ezUInt32 numBlocksY,
    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    const ezUInt32 targetWidth = numBlocksX * ezImageFormat::GetBlockWidth(targetFormat);
    const ezUInt32 targetHeight = numBlocksY * ezImageFormat::GetBlockHeight(targetFormat);

    Image srcImg;
    srcImg.width = targetWidth;
    srcImg.height = targetHeight;
    srcImg.rowPitch = ezImageFormat::GetRowPitch(sourceFormat, targetWidth);
    srcImg.slicePitch = ezImageFormat::GetDepthPitch(sourceFormat, targetWidth, targetHeight);
    srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(sourceFormat);
    srcImg.pixels = (uint8_t*)static_cast<const void*>(source.GetPtr());

    ScratchImage dxSrcImage;
    if (FAILED(dxSrcImage.InitializeFromImage(srcImg)))
      return EZ_FAILURE;

    const DXGI_FORMAT dxgiTargetFormat = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(targetFormat);

    ScratchImage dxDstImage;

    if (m_pD3dDevice == nullptr)
    {
      CreateDevice(0, &m_pD3dDevice);
    }

    bool bCompressionDone = false;

    if (m_pD3dDevice != nullptr)
    {
      if (SUCCEEDED(Compress(m_pD3dDevice, dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat,
            TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
      {
        // Not all formats can be compressed on the GPU. Fall back to CPU in case GPU compression fails.
        bCompressionDone = true;
      }
    }

    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat,
            TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }
    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat,
            TEX_COMPRESS_DEFAULT, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }

    if (!bCompressionDone)
      return EZ_FAILURE;

    target.CopyFrom(ezBlobPtr<const void>(dxDstImage.GetPixels(), static_cast<ezUInt32>(dxDstImage.GetPixelsSize())));

    return EZ_SUCCESS;
  }

  mutable ID3D11Device* m_pD3dDevice = nullptr;
};

static ezImageConversion_CompressDxTex s_conversion_compressDxTex;

#endif



EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexConversions);

