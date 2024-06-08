#include <Texture/TexturePCH.h>

#if EZ_USE_BC7ENC

#  include <bc7enc_rdo/rdo_bc_encoder.h>

#  include <Foundation/System/SystemInformation.h>
#  include <Texture/Image/ImageConversion.h>

ezImageConversionEntry g_BC7EncConversions[] = {
  // Even at the lowest quality level of BC7Enc, BC1 encoding times are more than a magnitude worse than DXTexConv.
  // ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC1_UNORM, ezImageConversionFlags::Default),
  // ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC1_UNORM_SRGB, ezImageConversionFlags::Default),
  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::BC7_UNORM, ezImageConversionFlags::Default),
  ezImageConversionEntry(ezImageFormat::R8G8B8A8_UNORM_SRGB, ezImageFormat::BC7_UNORM_SRGB, ezImageConversionFlags::Default),
};

class ezImageConversion_CompressBC7Enc : public ezImageConversionStepCompressBlocks
{
public:
  virtual ezArrayPtr<const ezImageConversionEntry> GetSupportedConversions() const override
  {
    return g_BC7EncConversions;
  }

  virtual ezResult CompressBlocks(ezConstByteBlobPtr source, ezByteBlobPtr target, ezUInt32 numBlocksX, ezUInt32 numBlocksY,
    ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat) const override
  {
    ezSystemInformation info = ezSystemInformation::Get();
    const ezInt32 iCpuCores = info.GetCPUCoreCount();

    rdo_bc::rdo_bc_params rp;
    rp.m_rdo_max_threads = ezMath::Clamp<ezInt32>(iCpuCores - 2, 2, 8);
    rp.m_status_output = false;
    rp.m_bc1_quality_level = 18;

    switch (targetFormat)
    {
      case ezImageFormat::BC7_UNORM:
      case ezImageFormat::BC7_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC7_UNORM;
        break;
      case ezImageFormat::BC1_UNORM:
      case ezImageFormat::BC1_UNORM_SRGB:
        rp.m_dxgi_format = DXGI_FORMAT_BC1_UNORM;
        break;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }

    utils::image_u8 source_image(numBlocksX * 4, numBlocksY * 4);
    auto& pixels = source_image.get_pixels();
    ezMemoryUtils::Copy<ezUInt32>(reinterpret_cast<ezUInt32*>(pixels.data()), reinterpret_cast<const ezUInt32*>(source.GetPtr()), numBlocksX * 4 * numBlocksY * 4);

    rdo_bc::rdo_bc_encoder encoder;
    if (!encoder.init(source_image, rp))
    {
      ezLog::Error("rdo_bc_encoder::init() failed!");
      return EZ_FAILURE;
    }

    if (!encoder.encode())
    {
      ezLog::Error("rdo_bc_encoder::encode() failed!");
      return EZ_FAILURE;
    }

    const ezUInt32 uiTotalBytes = encoder.get_total_blocks_size_in_bytes();
    if (uiTotalBytes != target.GetCount())
    {
      ezLog::Error("Encoder output of {} byte does not match the expected size of {} bytes", uiTotalBytes, target.GetCount());
      return EZ_FAILURE;
    }
    ezMemoryUtils::Copy<ezUInt8>(reinterpret_cast<ezUInt8*>(target.GetPtr()), reinterpret_cast<const ezUInt8*>(encoder.get_blocks()), uiTotalBytes);
    return EZ_SUCCESS;
  }
};

// EZ_STATICLINK_FORCE
static ezImageConversion_CompressBC7Enc s_conversion_compressBC7Enc;

#endif

EZ_STATICLINK_FILE(Texture, Texture_Image_Conversions_BC7EncConversions);
