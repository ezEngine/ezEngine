#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>


ezStbImageFileFormats g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to ezStreamReader

// namespace
//{
//  // fill 'data' with 'size' bytes.  return number of bytes actually read
//  int read(void *user, char *data, int size)
//  {
//    ezStreamReader* pStream = static_cast<ezStreamReader*>(user);
//    return static_cast<int>(pStream->ReadBytes(data, size));
//  }
//  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
//  void skip(void *user, int n)
//  {
//    ezStreamReader* pStream = static_cast<ezStreamReader*>(user);
//    if(n > 0)
//      pStream->SkipBytes(n);
//    else
//      // ?? We cannot reverse skip.
//
//  }
//  // returns nonzero if we are at end of file/data
//  int eof(void *user)
//  {
//    ezStreamReader* pStream = static_cast<ezStreamReader*>(user);
//    // ?
//  }
//}

namespace
{

  void write_func(void* pContext, void* pData, int iSize)
  {
    ezStreamWriter* writer = static_cast<ezStreamWriter*>(pContext);
    writer->WriteBytes(pData, iSize).IgnoreResult();
  }

  void* ReadImageData(ezStreamReader& inout_stream, ezDynamicArray<ezUInt8>& ref_fileBuffer, ezImageHeader& ref_imageHeader, bool& ref_bIsHDR)
  {
    ezStreamUtils::ReadAllAndAppend(inout_stream, ref_fileBuffer);

    int width, height, numComp;

    ref_bIsHDR = !!stbi_is_hdr_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount());

    void* sourceImageData = nullptr;
    if (ref_bIsHDR)
    {
      sourceImageData = stbi_loadf_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      sourceImageData = stbi_load_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!sourceImageData)
    {
      ezLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    ref_fileBuffer.Clear();

    ezImageFormat::Enum format = ezImageFormat::UNKNOWN;
    switch (numComp)
    {
      case 1:
        format = (ref_bIsHDR) ? ezImageFormat::R32_FLOAT : ezImageFormat::R8_UNORM;
        break;
      case 2:
        format = (ref_bIsHDR) ? ezImageFormat::R32G32_FLOAT : ezImageFormat::R8G8_UNORM;
        break;
      case 3:
        format = (ref_bIsHDR) ? ezImageFormat::R32G32B32_FLOAT : ezImageFormat::R8G8B8_UNORM;
        break;
      case 4:
        format = (ref_bIsHDR) ? ezImageFormat::R32G32B32A32_FLOAT : ezImageFormat::R8G8B8A8_UNORM;
        break;
    }

    // Set properties and allocate.
    ref_imageHeader.SetImageFormat(format);
    ref_imageHeader.SetNumMipLevels(1);
    ref_imageHeader.SetNumArrayIndices(1);
    ref_imageHeader.SetNumFaces(1);

    ref_imageHeader.SetWidth(width);
    ref_imageHeader.SetHeight(height);
    ref_imageHeader.SetDepth(1);

    return sourceImageData;
  }

} // namespace

ezResult ezStbImageFileFormats::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, const char* szFileExtension) const
{
  EZ_PROFILE_SCOPE("ezStbImageFileFormats::ReadImageHeader");

  bool isHDR = false;
  ezDynamicArray<ezUInt8> fileBuffer;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, ref_header, isHDR);

  if (sourceImageData == nullptr)
    return EZ_FAILURE;

  stbi_image_free(sourceImageData);
  return EZ_SUCCESS;
}

ezResult ezStbImageFileFormats::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, const char* szFileExtension) const
{
  EZ_PROFILE_SCOPE("ezStbImageFileFormats::ReadImage");

  bool isHDR = false;
  ezDynamicArray<ezUInt8> fileBuffer;
  ezImageHeader imageHeader;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, imageHeader, isHDR);

  if (sourceImageData == nullptr)
    return EZ_FAILURE;

  ref_image.ResetAndAlloc(imageHeader);

  const size_t numComp = ezImageFormat::GetNumChannels(imageHeader.GetImageFormat());

  const size_t elementsToCopy = static_cast<size_t>(imageHeader.GetWidth()) * static_cast<size_t>(imageHeader.GetHeight()) * numComp;

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = ref_image.GetBlobPtr<float>().GetPtr();
    ezMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, elementsToCopy);
  }
  else
  {
    ezUInt8* targetImageData = ref_image.GetBlobPtr<ezUInt8>().GetPtr();
    ezMemoryUtils::Copy(targetImageData, (const ezUInt8*)sourceImageData, elementsToCopy);
  }

  stbi_image_free((void*)sourceImageData);
  return EZ_SUCCESS;
}

ezResult ezStbImageFileFormats::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, const char* szFileExtension) const
{
  ezImageFormat::Enum compatibleFormats[] = {ezImageFormat::R8_UNORM, ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", ezImageFormat::GetName(image.GetImageFormat()));
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

    return WriteImage(inout_stream, convertedImage, szFileExtension);
  }

  if (ezStringUtils::IsEqual_NoCase(szFileExtension, "png"))
  {
    if (stbi_write_png_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), ezImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return EZ_SUCCESS;
    }
  }

  if (ezStringUtils::IsEqual_NoCase(szFileExtension, "jpg") || ezStringUtils::IsEqual_NoCase(szFileExtension, "jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), ezImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

bool ezStbImageFileFormats::CanReadFileType(const char* szExtension) const
{
  if (ezStringUtils::IsEqual_NoCase(szExtension, "hdr"))
    return true;

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

  // on Windows Desktop, we prefer to use WIC (ezWicFileFormat)
  if (ezStringUtils::IsEqual_NoCase(szExtension, "png") || ezStringUtils::IsEqual_NoCase(szExtension, "jpg") || ezStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool ezStbImageFileFormats::CanWriteFileType(const char* szExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (ezStringUtils::IsEqual_NoCase(szExtension, "png") || ezStringUtils::IsEqual_NoCase(szExtension, "jpg") || ezStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }

  return false;
}



EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);
