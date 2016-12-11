#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Formats/StbImageFileFormats.h>
#include <CoreUtils/Image/Image.h>

#include <ThirdParty/stb_image/stb_image.h>


ezStbImageFileFormats g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to ezStreamReader

//namespace
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

ezResult ezStbImageFileFormats::ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog) const
{
  //stbi_io_callbacks callbacks;
  //callbacks.read = &read;
  //callbacks.skip = &skip;
  //callbacks.eof = &eof;

  ezMemoryStreamStorage fileBuffer;
  fileBuffer.ReadAll(stream);

  int width, height, numComp;
  stbi_uc* sourceImageData = stbi_load_from_memory(fileBuffer.GetData(), fileBuffer.GetStorageSize(), &width, &height, &numComp, 0);
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
    format = ezImageFormat::R8_UNORM;
    break;
  case 2:
  case 3:
  case 4:
    // No 2 or 3 component format, need to convert.
    format = ezImageFormat::R8G8B8A8_UNORM;
    break;
  }

  // Set properties and allocate.
  image.SetImageFormat(format);
  image.SetNumMipLevels(1);
  image.SetNumArrayIndices(1);
  image.SetNumFaces(1);

  image.SetWidth(width);
  image.SetHeight(height);
  image.SetDepth(1);

  image.AllocateImageData();

  // Set pixels. Different strategies depending on component count.
  unsigned char* targetImageData = image.GetDataPointer<unsigned char>();
  int numPixels = width * height;
  switch (numComp)
  {
  case 1:
    ezMemoryUtils::Copy(targetImageData, sourceImageData, width * height);
    break;

  case 2:
  {
    unsigned char* currentSourceImageData = sourceImageData;
    for (int i = 0; i < numPixels; ++i)
    {
      *targetImageData = *currentSourceImageData;
      *(++targetImageData) = *(++currentSourceImageData);
      *(++targetImageData) = 0;
      *(++targetImageData) = 0;

      ++targetImageData;
      ++currentSourceImageData;
    }
    break;
  }

  case 3:
  {
    unsigned char* currentSourceImageData = sourceImageData;
    for (int i = 0; i < numPixels; ++i)
    {
      *targetImageData = *currentSourceImageData;
      *(++targetImageData) = *(++currentSourceImageData);
      *(++targetImageData) = *(++currentSourceImageData);
      *(++targetImageData) = 0;

      ++targetImageData;
      ++currentSourceImageData;
    }
    break;
  }

  case 4:
    ezMemoryUtils::Copy(targetImageData, sourceImageData, width * height * 4);
    break;
  }


  stbi_image_free(sourceImageData);

  return EZ_SUCCESS;
}

ezResult ezStbImageFileFormats::WriteImage(ezStreamWriter& stream, const ezImage& image, ezLogInterface* pLog) const
{
  return EZ_FAILURE;
}

bool ezStbImageFileFormats::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "png") ||
         ezStringUtils::IsEqual_NoCase(szExtension, "jpg") ||
         ezStringUtils::IsEqual_NoCase(szExtension, "jpeg");
}

bool ezStbImageFileFormats::CanWriteFileType(const char* szExtension) const
{
  return false;
}

EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Formats_StbImageFileFormats);
