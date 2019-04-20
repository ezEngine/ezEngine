#include <TexturePCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Formats/WicFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <DirectXTex/DirectXTex.h>

EZ_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in ez containers

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

ezResult ezWicFileFormat::ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  using namespace DirectX;

  // Read image data into local storage for further processing
  constexpr int tempStorageSize = 4096;
  ezDynamicArray<ezUInt8> storage;
  ezUInt8 tempStorage[tempStorageSize];

  while (ezUInt64 bytesRead = stream.ReadBytes(tempStorage, tempStorageSize))
  {
    storage.PushBackRange(ezArrayPtr<ezUInt8>(tempStorage, static_cast<ezUInt32>(bytesRead)));
  }

  if (storage.IsEmpty())
  {
    ezLog::Error(pLog, "Failure to retrieve image data.");
    return EZ_FAILURE;
  }

  // Read WIC data from local storage
  ScratchImage scratchImage;
  DWORD flags = WIC_FLAGS_ALL_FRAMES;
  if (FAILED(LoadFromWICMemory(storage.GetData(), storage.GetCount(), flags, nullptr, scratchImage)))
  {
    ezLog::Error(pLog, "Failure to process image data.");
    return EZ_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  const TexMetadata& metadata = scratchImage.GetMetadata();
  ezImageFormat::Enum imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Warning(pLog, "Unable to use image format from TIFF file - trying conversion.");
    flags |= WIC_FLAGS_FORCE_RGB;
    LoadFromWICMemory(storage.GetData(), storage.GetCount(), flags, nullptr, scratchImage);
    imageFormat = ezImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(pLog, "Unable to use image format from TIFF file.");
    return EZ_FAILURE;
  }

  // Prepare destination image header and allocate storage
  ezImageHeader imageHeader;
  imageHeader.SetImageFormat(imageFormat);

  imageHeader.SetWidth(ezUInt32(metadata.width));
  imageHeader.SetHeight(ezUInt32(metadata.height));
  imageHeader.SetDepth(ezUInt32(metadata.depth));

  imageHeader.SetNumMipLevels(1);
  imageHeader.SetNumArrayIndices(ezUInt32(metadata.IsCubemap() ? (metadata.arraySize / 6) : metadata.arraySize));
  imageHeader.SetNumFaces(metadata.IsCubemap() ? 6 : 1);

  image.ResetAndAlloc(imageHeader);

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
        ezUInt8* destPixels = image.GetPixelPointer<ezUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (sourceImage && destPixels && sourceImage->pixels)
        {
          if (destRowPitch == sourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            ezMemoryUtils::Copy(destPixels, sourceImage->pixels, imageHeader.GetHeight() * destRowPitch);
          }
          else
          {
            // Row pitches don't match - copy row by row
            ezUInt64 bytesPerRow = ezMath::Min(destRowPitch, sourceImage->rowPitch);
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

ezResult ezWicFileFormat::WriteImage(
  ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const
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
    ezLog::Error(
      pLog, "No conversion from format '{0}' to a format suitable for TIFF files known.", ezImageFormat::GetName(image.GetImageFormat()));
    return EZ_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    ezImage convertedImage;
    if (ezImageConversion::Convert(image, convertedImage, format) != EZ_SUCCESS)
    {
      // This should never happen
      EZ_ASSERT_DEV(
        false, "ezImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return EZ_FAILURE;
    }

    return WriteImage(stream, convertedImage, pLog, szFileExtension);
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
        currentImage.rowPitch = image.GetRowPitch();
        currentImage.slicePitch = image.GetDepthPitch();
        currentImage.pixels = const_cast<uint8_t*>(image.GetPixelPointer<uint8_t>(0, faceIdx, arrayIdx, 0, 0, sliceIdx));
      }
    }
  }

  if (!outputImages.IsEmpty())
  {
    // Store images in output blob
    Blob targetBlob;
    DWORD flags = WIC_FLAGS_NONE;
    HRESULT res = SaveToWICMemory(outputImages.GetData(), outputImages.GetCount(), flags, GetWICCodec(WIC_CODEC_TIFF), targetBlob);
    if (FAILED(res))
    {
      ezLog::Error(pLog, "Failed to save image data to local memory blob - result: {}!", res);
      return EZ_FAILURE;
    }

    // Push blob into output stream
    if (stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write image data!");
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezWicFileFormat::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "tif") || ezStringUtils::IsEqual_NoCase(szExtension, "tiff");
}

bool ezWicFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);

#endif
