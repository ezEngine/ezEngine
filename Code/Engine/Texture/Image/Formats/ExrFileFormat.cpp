#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/ExrFileFormat.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>

#include <tinyexr/tinyexr.h>

ezExrFileFormat g_ExrFileFormat;

ezResult ReadImageData(ezStreamReader& stream, ezDynamicArray<ezUInt8>& fileBuffer, ezImageHeader& header, EXRHeader& exrHeader, EXRImage& exrImage, bool& isHDR)
{
  // Open memory
  ezStreamUtils::ReadAllAndAppend(stream, fileBuffer);

  // 1. Read EXR version.
  EXRVersion exrVersion;

  int ret = ParseEXRVersionFromMemory(&exrVersion, fileBuffer.GetData(), fileBuffer.GetCount());
  if (ret != 0)
  {
    ezLog::Error("Invalid EXR file: Cannot read version from file.");
    return EZ_FAILURE;
  }

  if (exrVersion.multipart)
  {
    // TODO: Support multipart formats
    ezLog::Error("Invalid EXR file: Multipart formats are unsupported.");
    return EZ_FAILURE;
  }

  // 2. Read EXR header
  const char* err = nullptr;
  ret = ParseEXRHeaderFromMemory(&exrHeader, &exrVersion, fileBuffer.GetData(), fileBuffer.GetCount(), &err);
  if (ret != 0)
  {
    ezLog::Error("Unable to read EXR file header: {0}.", err);
    FreeEXRErrorMessage(err);
    return EZ_FAILURE;
  }

  ret = LoadEXRImageFromMemory(&exrImage, &exrHeader, fileBuffer.GetData(), fileBuffer.GetCount(), &err);
  if (ret != 0)
  {
    ezLog::Error("Unable to load EXR file: {0}.", err);
    FreeEXRHeader(&exrHeader);
    FreeEXRErrorMessage(err);
    return EZ_FAILURE;
  }

  ezImageFormat::Enum imageFormat;

  switch (exrHeader.num_channels)
  {
    case 1:
      imageFormat = exrHeader.pixel_types[0] == TINYEXR_PIXELTYPE_FLOAT ? ezImageFormat::R32_FLOAT : exrHeader.pixel_types[0] == TINYEXR_PIXELTYPE_HALF ? ezImageFormat::R16_FLOAT
                                                                                                                                                        : ezImageFormat::R32_UINT;
      break;

    case 3:
      if (exrHeader.pixel_types[0] != exrHeader.pixel_types[1] || exrHeader.pixel_types[0] != exrHeader.pixel_types[2])
      {
        ezLog::Error("Not supported EXR file: all channels should have the same size.");
        return EZ_FAILURE;
      }

      imageFormat = exrHeader.pixel_types[0] == TINYEXR_PIXELTYPE_UINT ? ezImageFormat::R32G32B32_UINT : ezImageFormat::R32G32B32_FLOAT;
      break;

    case 4:
      if (exrHeader.pixel_types[0] != exrHeader.pixel_types[1] || exrHeader.pixel_types[0] != exrHeader.pixel_types[2] || exrHeader.pixel_types[0] != exrHeader.pixel_types[3])
      {
        ezLog::Error("Not supported EXR file: all channels should have the same size.");
        return EZ_FAILURE;
      }

      imageFormat = exrHeader.pixel_types[0] == TINYEXR_PIXELTYPE_FLOAT ? ezImageFormat::R32G32B32A32_FLOAT : exrHeader.pixel_types[0] == TINYEXR_PIXELTYPE_HALF ? ezImageFormat::R16G16B16A16_FLOAT
                                                                                                                                                                 : ezImageFormat::R32G32B32A32_UINT;
      break;

    default:
      ezLog::Error("Not supported EXR file: only 1, 3 and 4 channels count are supported.");
      return EZ_FAILURE;
  }

  // 3. Setup header
  header.SetHeight(exrImage.height);
  header.SetImageFormat(imageFormat);
  header.SetWidth(exrImage.width);

  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);
  header.SetDepth(1);

  isHDR = imageFormat == ezImageFormat::R32_FLOAT ||
          imageFormat == ezImageFormat::R16_FLOAT ||
          imageFormat == ezImageFormat::R32G32_FLOAT ||
          imageFormat == ezImageFormat::R16G16_FLOAT ||
          imageFormat == ezImageFormat::R32G32B32A32_FLOAT ||
          imageFormat == ezImageFormat::R16G16B16A16_FLOAT;

  return EZ_SUCCESS;
}

ezResult ezExrFileFormat::ReadImageHeader(ezStreamReader& stream, ezImageHeader& header, const char* szFileExtension) const
{
  EZ_PROFILE_SCOPE("ezExrFileFormat::ReadImageHeader");

  ezDynamicArray<ezUInt8> fileBuffer;

  bool isHdr = false;

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);

  EXRImage exrImage;
  InitEXRImage(&exrImage);

  if (ReadImageData(stream, fileBuffer, header, exrHeader, exrImage, isHdr).Failed())
    return EZ_FAILURE;

  // Free image data
  FreeEXRImage(&exrImage);
  FreeEXRHeader(&exrHeader);

  return EZ_SUCCESS;
}

ezResult ezExrFileFormat::ReadImage(ezStreamReader& stream, ezImage& image, const char* szFileExtension) const
{
  EZ_PROFILE_SCOPE("ezExrFileFormat::ReadImage");

  ezDynamicArray<ezUInt8> fileBuffer;

  bool isHDR = false;

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);

  EXRImage exrImage;
  InitEXRImage(&exrImage);

  ezImageHeader header;

  if (ReadImageData(stream, fileBuffer, header, exrHeader, exrImage, isHDR).Failed())
    return EZ_FAILURE;

  image.ResetAndAlloc(header);

  const ezUInt32 pixelCount = header.GetWidth() * header.GetHeight();
  const ezUInt32 numComp = ezImageFormat::GetNumChannels(header.GetImageFormat());
  const ezUInt32 elementsToCopy = header.GetWidth() * header.GetHeight() * numComp;

  // Copy the data over to our vector
  if (numComp == 1)
  {
    ezUInt8* bytes = exrImage.images[0];

    // Set pixels. Different strategies depending on component count.
    if (isHDR)
    {
      float* targetImageData = image.GetBlobPtr<float>().GetPtr();
      ezMemoryUtils::Copy(targetImageData, reinterpret_cast<const float*>(bytes), elementsToCopy);
    }
    else
    {
      ezUInt32* targetImageData = image.GetBlobPtr<ezUInt32>().GetPtr();
      ezMemoryUtils::Copy(targetImageData, reinterpret_cast<const ezUInt32*>(bytes), elementsToCopy);
    }
  }
  else if (numComp >= 3)
  {
    ezUInt32 c = 0;

    ezUInt8* aBytes = nullptr;
    if (numComp == 4)
      aBytes = exrImage.images[c++];

    ezUInt8* bBytes = exrImage.images[c++];
    ezUInt8* gBytes = exrImage.images[c++];
    ezUInt8* rBytes = exrImage.images[c++];

    EZ_ASSERT_DEV(numComp == c, "Built number of channels is different than the channel count");

    // Set pixels. Different strategies depending on component count.
    if (isHDR)
    {
      float* targetImageData = image.GetBlobPtr<float>().GetPtr();
      for (ezUInt32 i = 0, j = 0; i < pixelCount; ++i, j += numComp)
      {
        targetImageData[j + 0] = reinterpret_cast<float*>(rBytes)[i];
        targetImageData[j + 1] = reinterpret_cast<float*>(gBytes)[i];
        targetImageData[j + 2] = reinterpret_cast<float*>(bBytes)[i];

        if (numComp == 4)
          targetImageData[j + 3] = reinterpret_cast<float*>(aBytes)[i];
      }
    }
    else
    {
      ezUInt32* targetImageData = image.GetBlobPtr<ezUInt32>().GetPtr();
      for (ezUInt32 i = 0, j = 0; i < pixelCount; ++i, j += numComp)
      {
        targetImageData[j + 0] = reinterpret_cast<ezUInt32*>(rBytes)[i];
        targetImageData[j + 1] = reinterpret_cast<ezUInt32*>(gBytes)[i];
        targetImageData[j + 2] = reinterpret_cast<ezUInt32*>(bBytes)[i];

        if (numComp == 4)
          targetImageData[j + 3] = reinterpret_cast<ezUInt32*>(aBytes)[i];
      }
    }
  }

  // Free image data
  FreeEXRImage(&exrImage);
  FreeEXRHeader(&exrHeader);

  return EZ_SUCCESS;
}

ezResult ezExrFileFormat::WriteImage(ezStreamWriter& stream, const ezImageView& image, const char* szFileExtension) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

bool ezExrFileFormat::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "exr");
}

bool ezExrFileFormat::CanWriteFileType(const char* szExtension) const
{
  // TODO: Write EXR files
  return false;
}

EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);
