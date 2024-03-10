#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Formats/WicFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Texture/DirectXTex/DirectXTex.h>

using namespace DirectX;

EZ_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in ez containers

// EZ_STATICLINK_FORCE
ezWicFileFormat g_wicFormat;

namespace
{
  /// \brief Try to init COM, return true if we are the first(!) to successfully do so
  bool InitializeCOM()
  {
    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK)
    {
      // We were the first one - deinit on shutdown
      return true;
    }
    else if (SUCCEEDED(result))
    {
      // We were not the first one, but we still succeeded, so we deinit COM right away.
      // Otherwise we might be the last one to call CoUninitialize(), but that is supposed to be the one who called
      // CoInitialize[Ex]() first.
      CoUninitialize();
    }

    // We won't call CoUninitialize() on shutdown as either we were not the first one to init COM successfully (and uninitialized it right away),
    // or our call to CoInitializeEx() didn't succeed, because it was already called with another concurrency model specifier.
    return false;
  }
} // namespace


ezWicFileFormat::ezWicFileFormat() = default;

ezWicFileFormat::~ezWicFileFormat()
{
  if (m_bCoUninitOnShutdown)
  {
    // We were the first one to successfully initialize COM, so we are the one who needs to shut it down.
    CoUninitialize();
  }
}

ezResult ezWicFileFormat::ReadFileData(ezStreamReader& stream, ezDynamicArray<ezUInt8>& storage) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  ezStreamUtils::ReadAllAndAppend(stream, storage);

  if (storage.IsEmpty())
  {
    ezLog::Error("Failure to retrieve image data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

static void SetHeader(ezImageHeader& ref_header, ezImageFormat::Enum imageFormat, const TexMetadata& metadata)
{
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetWidth(ezUInt32(metadata.width));
  ref_header.SetHeight(ezUInt32(metadata.height));
  ref_header.SetDepth(ezUInt32(metadata.depth));

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(ezUInt32(metadata.IsCubemap() ? (metadata.arraySize / 6) : metadata.arraySize));
  ref_header.SetNumFaces(metadata.IsCubemap() ? 6 : 1);
}

ezResult ezWicFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_PROFILE_SCOPE("ezWicFileFormat::ReadImageHeader");

  ezDynamicArray<ezUInt8> storage;
  EZ_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  HRESULT loadResult = GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
  if (FAILED(loadResult))
  {
    ezLog::Error("Failure to load image metadata. HRESULT:{}", ezArgErrorCode(loadResult));
    return EZ_FAILURE;
  }

  ezImageFormat::Enum imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
    imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Unable to use image format from '{}' file.", sFileExtension);
    return EZ_FAILURE;
  }

  SetHeader(ref_header, imageFormat, metadata);

  return EZ_SUCCESS;
}

ezResult ezWicFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_PROFILE_SCOPE("ezWicFileFormat::ReadImage");

  ezDynamicArray<ezUInt8> storage;
  EZ_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  // Read WIC data from local storage
  HRESULT loadResult = LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
  if (FAILED(loadResult))
  {
    ezLog::Error("Failure to load image data. HRESULT:{}", ezArgErrorCode(loadResult));
    return EZ_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  metadata = scratchImage.GetMetadata();

  ezImageFormat::Enum imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
    imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Unable to use image format from '{}' file.", sFileExtension);
    return EZ_FAILURE;
  }

  // Prepare destination image header and allocate storage
  ezImageHeader imageHeader;
  SetHeader(imageHeader, imageFormat, metadata);

  ref_image.ResetAndAlloc(imageHeader);

  // Read image data into destination image
  ezUInt64 destRowPitch = imageHeader.GetRowPitch();
  ezUInt32 itemIdx = 0;
  for (ezUInt32 arrayIdx = 0; arrayIdx < imageHeader.GetNumArrayIndices(); ++arrayIdx)
  {
    for (ezUInt32 faceIdx = 0; faceIdx < imageHeader.GetNumFaces(); ++faceIdx, ++itemIdx)
    {
      for (ezUInt32 sliceIdx = 0; sliceIdx < imageHeader.GetDepth(); ++sliceIdx)
      {
        const Image* sourceImage = scratchImage.GetImage(0, itemIdx, sliceIdx);
        ezUInt8* destPixels = ref_image.GetPixelPointer<ezUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (sourceImage && destPixels && sourceImage->pixels)
        {
          if (destRowPitch == sourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            ezMemoryUtils::Copy(destPixels, sourceImage->pixels, static_cast<size_t>(imageHeader.GetHeight() * destRowPitch));
          }
          else
          {
            // Row pitches don't match - copy row by row
            ezUInt64 bytesPerRow = ezMath::Min(destRowPitch, ezUInt64(sourceImage->rowPitch));
            const uint8_t* sourcePixels = sourceImage->pixels;
            for (ezUInt32 rowIdx = 0; rowIdx < imageHeader.GetHeight(); ++rowIdx)
            {
              ezMemoryUtils::Copy(destPixels, sourcePixels, static_cast<size_t>(bytesPerRow));

              destPixels += destRowPitch;
              sourcePixels += sourceImage->rowPitch;
            }
          }
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezWicFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  using namespace DirectX;

  // Convert into suitable output format
  ezImageFormat::Enum compatibleFormats[] = {
    ezImageFormat::R8G8B8A8_UNORM,
    ezImageFormat::R8G8B8A8_UNORM_SRGB,
    ezImageFormat::R8_UNORM,
    ezImageFormat::R16G16B16A16_UNORM,
    ezImageFormat::R16_UNORM,
    ezImageFormat::R32G32B32A32_FLOAT,
    ezImageFormat::R32G32B32_FLOAT,
  };

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("No conversion from format '{0}' to a format suitable for '{}' files known.", ezImageFormat::GetName(image.GetImageFormat()), sFileExtension);
    return EZ_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    ezImage convertedImage;
    if (ezImageConversion::Convert(image, convertedImage, format) != EZ_SUCCESS)
    {
      // This should never happen
      EZ_ASSERT_DEV(false, "ezImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return EZ_FAILURE;
    }

    return WriteImage(inout_stream, convertedImage, sFileExtension);
  }

  // Store ezImage data in DirectXTex images
  ezDynamicArray<Image> outputImages;
  DXGI_FORMAT imageFormat = DXGI_FORMAT(ezImageFormatMappings::ToDxgiFormat(image.GetImageFormat()));
  for (ezUInt32 arrayIdx = 0; arrayIdx < image.GetNumArrayIndices(); ++arrayIdx)
  {
    for (ezUInt32 faceIdx = 0; faceIdx < image.GetNumFaces(); ++faceIdx)
    {
      for (ezUInt32 sliceIdx = 0; sliceIdx < image.GetDepth(); ++sliceIdx)
      {
        Image& currentImage = outputImages.ExpandAndGetRef();
        currentImage.width = image.GetWidth();
        currentImage.height = image.GetHeight();
        currentImage.format = imageFormat;
        currentImage.rowPitch = static_cast<size_t>(image.GetRowPitch());
        currentImage.slicePitch = static_cast<size_t>(image.GetDepthPitch());
        currentImage.pixels = const_cast<uint8_t*>(image.GetPixelPointer<uint8_t>(0, faceIdx, arrayIdx, 0, 0, sliceIdx));
      }
    }
  }

  if (!outputImages.IsEmpty())
  {
    // Store images in output blob
    Blob targetBlob;
    WIC_FLAGS flags = WIC_FLAGS_NONE;
    HRESULT res = SaveToWICMemory(outputImages.GetData(), outputImages.GetCount(), flags, GetWICCodec(WIC_CODEC_TIFF), targetBlob);
    if (FAILED(res))
    {
      ezLog::Error("Failed to save image data to local memory blob - result: {}!", ezHRESULTtoString(res));
      return EZ_FAILURE;
    }

    // Push blob into output stream
    if (inout_stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != EZ_SUCCESS)
    {
      ezLog::Error("Failed to write image data!");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezWicFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg") ||
         sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

bool ezWicFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  // png, jpg and jpeg are handled by STB (ezStbImageFileFormats)
  return sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

#endif



EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);
