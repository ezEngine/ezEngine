#include <PCH.h>

#include <Foundation/Image/Formats/StbImageFileFormats.h>
#include <Foundation/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Image/ImageConversion.h>
#include <ThirdParty/stb_image/stb_image.h>
#include <ThirdParty/stb_image/stb_image_write.h>


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

  void write_func(void* context, void* data, int size)
  {
    ezStreamWriter* writer = static_cast<ezStreamWriter*>(context);
    writer->WriteBytes(data, size);
  }

} // namespace

ezResult ezStbImageFileFormats::ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const
{
  // stbi_io_callbacks callbacks;
  // callbacks.read = &read;
  // callbacks.skip = &skip;
  // callbacks.eof = &eof;

  ezMemoryStreamStorage fileBuffer;
  fileBuffer.ReadAll(stream);

  int width, height, numComp;

  bool isHDR = !!stbi_is_hdr_from_memory(fileBuffer.GetData(), fileBuffer.GetStorageSize());

  const void* sourceImageData = nullptr;
  if (isHDR)
  {
    sourceImageData = stbi_loadf_from_memory(fileBuffer.GetData(), fileBuffer.GetStorageSize(), &width, &height, &numComp, 0);
  }
  else
  {
    sourceImageData = stbi_load_from_memory(fileBuffer.GetData(), fileBuffer.GetStorageSize(), &width, &height, &numComp, 0);
  }
  if (!sourceImageData)
  {
    ezLog::Error(pLog, "stb_image failed to load: {0}", stbi_failure_reason());
    return EZ_FAILURE;
  }
  fileBuffer.Clear();

  ezImageFormat::Enum format = ezImageFormat::UNKNOWN;
  switch (numComp)
  {
    case 1:
      format = (isHDR) ? ezImageFormat::R32_FLOAT : ezImageFormat::R8_UNORM;
      break;
    case 2:
      format = (isHDR) ? ezImageFormat::R32G32_FLOAT : ezImageFormat::R8G8_UNORM;
      break;
    case 3:
      format = (isHDR) ? ezImageFormat::R32G32B32_FLOAT : ezImageFormat::R8G8B8_UNORM;
      break;
    case 4:
      format = (isHDR) ? ezImageFormat::R32G32B32A32_FLOAT : ezImageFormat::R8G8B8A8_UNORM;
      break;
  }

  // Set properties and allocate.
  ezImageHeader imageHeader;
  imageHeader.SetImageFormat(format);
  imageHeader.SetNumMipLevels(1);
  imageHeader.SetNumArrayIndices(1);
  imageHeader.SetNumFaces(1);

  imageHeader.SetWidth(width);
  imageHeader.SetHeight(height);
  imageHeader.SetDepth(1);

  image.ResetAndAlloc(imageHeader);

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = image.GetArrayPtr<float>().GetPtr();
    ezMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, width * height * numComp);
  }
  else
  {
    ezUInt8* targetImageData = image.GetArrayPtr<ezUInt8>().GetPtr();
    ezMemoryUtils::Copy(targetImageData, (const ezUInt8*)sourceImageData, width * height * numComp);
  }

  stbi_image_free((void*)sourceImageData);

  return EZ_SUCCESS;
}

ezResult ezStbImageFileFormats::WriteImage(
  ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const
{
  ezImageFormat::Enum compatibleFormats[] = {ezImageFormat::R8_UNORM, ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(
      pLog, "No conversion from format '{0}' to a format suitable for PNG files known.", ezImageFormat::GetName(image.GetImageFormat()));
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

  if (ezStringUtils::IsEqual_NoCase(szFileExtension, "png"))
  {
    if (stbi_write_png_to_func(write_func, &stream, image.GetWidth(), image.GetHeight(),
          ezImageFormat::GetNumChannels(image.GetImageFormat()), image.GetArrayPtr<void>().GetPtr(), 0))
    {
      return EZ_SUCCESS;
    }
  }

  if (ezStringUtils::IsEqual_NoCase(szFileExtension, "jpg") || ezStringUtils::IsEqual_NoCase(szFileExtension, "jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &stream, image.GetWidth(), image.GetHeight(),
          ezImageFormat::GetNumChannels(image.GetImageFormat()), image.GetArrayPtr<void>().GetPtr(), 95))
    {
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

bool ezStbImageFileFormats::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "png") || ezStringUtils::IsEqual_NoCase(szExtension, "jpg") ||
         ezStringUtils::IsEqual_NoCase(szExtension, "jpeg") || ezStringUtils::IsEqual_NoCase(szExtension, "hdr");
}

bool ezStbImageFileFormats::CanWriteFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "png") || ezStringUtils::IsEqual_NoCase(szExtension, "jpg") ||
         ezStringUtils::IsEqual_NoCase(szExtension, "jpeg");

}



EZ_STATICLINK_FILE(Foundation, Foundation_Image_Formats_StbImageFileFormats);
