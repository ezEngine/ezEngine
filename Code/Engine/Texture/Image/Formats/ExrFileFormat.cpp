#include <Texture/TexturePCH.h>

#ifdef BUILDSYSTEM_ENABLE_TINYEXR_SUPPORT

#  include <Texture/Image/Formats/ExrFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <tinyexr/tinyexr.h>

// EZ_STATICLINK_FORCE
ezExrFileFormat g_ExrFileFormat;

ezResult ReadImageData(ezStreamReader& ref_stream, ezDynamicArray<ezUInt8>& ref_fileBuffer, ezImageHeader& ref_header, EXRHeader& ref_exrHeader, EXRImage& ref_exrImage)
{
  // read the entire file to memory
  ezStreamUtils::ReadAllAndAppend(ref_stream, ref_fileBuffer);

  // read the EXR version
  EXRVersion exrVersion;

  if (ParseEXRVersionFromMemory(&exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount()) != 0)
  {
    ezLog::Error("Invalid EXR file: Cannot read version.");
    return EZ_FAILURE;
  }

  if (exrVersion.multipart)
  {
    ezLog::Error("Invalid EXR file: Multi-part formats are not supported.");
    return EZ_FAILURE;
  }

  // read the EXR header
  const char* err = nullptr;
  if (ParseEXRHeaderFromMemory(&ref_exrHeader, &exrVersion, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    ezLog::Error("Invalid EXR file: '{0}'", err);
    FreeEXRErrorMessage(err);
    return EZ_FAILURE;
  }

  for (int c = 1; c < ref_exrHeader.num_channels; ++c)
  {
    if (ref_exrHeader.pixel_types[c - 1] != ref_exrHeader.pixel_types[c])
    {
      ezLog::Error("Unsupported EXR file: all channels should have the same size.");
      break;
    }
  }

  if (LoadEXRImageFromMemory(&ref_exrImage, &ref_exrHeader, ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &err) != 0)
  {
    ezLog::Error("Invalid EXR file: '{0}'", err);

    FreeEXRHeader(&ref_exrHeader);
    FreeEXRErrorMessage(err);
    return EZ_FAILURE;
  }

  ezImageFormat::Enum imageFormat = ezImageFormat::UNKNOWN;

  switch (ref_exrHeader.num_channels)
  {
    case 1:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = ezImageFormat::R32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = ezImageFormat::R16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = ezImageFormat::R32_UINT;
          break;
      }

      break;
    }

    case 2:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = ezImageFormat::R32G32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = ezImageFormat::R16G16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = ezImageFormat::R32G32_UINT;
          break;
      }

      break;
    }

    case 3:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = ezImageFormat::R32G32B32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = ezImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = ezImageFormat::R32G32B32_UINT;
          break;
      }

      break;
    }

    case 4:
    {
      switch (ref_exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = ezImageFormat::R32G32B32A32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = ezImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = ezImageFormat::R32G32B32A32_UINT;
          break;
      }

      break;
    }
  }

  if (imageFormat == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Unsupported EXR file: {}-channel files with format '{}' are unsupported.", ref_exrHeader.num_channels, ref_exrHeader.pixel_types[0]);
    return EZ_FAILURE;
  }

  ref_header.SetWidth(ref_exrImage.width);
  ref_header.SetHeight(ref_exrImage.height);
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);
  ref_header.SetDepth(1);

  return EZ_SUCCESS;
}

ezResult ezExrFileFormat::ReadImageHeader(ezStreamReader& ref_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  EZ_PROFILE_SCOPE("ezExrFileFormat::ReadImageHeader");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  EZ_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  EZ_SCOPE_EXIT(FreeEXRImage(&exrImage));

  ezDynamicArray<ezUInt8> fileBuffer;
  return ReadImageData(ref_stream, fileBuffer, ref_header, exrHeader, exrImage);
}

static void CopyChannel(ezUInt8* pDst, const ezUInt8* pSrc, ezUInt32 uiNumElements, ezUInt32 uiElementSize, ezUInt32 uiDstStride)
{
  if (uiDstStride == uiElementSize)
  {
    // fast path to copy everything in one operation
    // this only happens for single-channel formats
    ezMemoryUtils::RawByteCopy(pDst, pSrc, uiNumElements * uiElementSize);
  }
  else
  {
    for (ezUInt32 i = 0; i < uiNumElements; ++i)
    {
      ezMemoryUtils::RawByteCopy(pDst, pSrc, uiElementSize);

      pSrc = ezMemoryUtils::AddByteOffset(pSrc, uiElementSize);
      pDst = ezMemoryUtils::AddByteOffset(pDst, uiDstStride);
    }
  }
}

ezResult ezExrFileFormat::ReadImage(ezStreamReader& ref_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  EZ_PROFILE_SCOPE("ezExrFileFormat::ReadImage");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  EZ_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  EZ_SCOPE_EXIT(FreeEXRImage(&exrImage));

  ezImageHeader header;
  ezDynamicArray<ezUInt8> fileBuffer;

  EZ_SUCCEED_OR_RETURN(ReadImageData(ref_stream, fileBuffer, header, exrHeader, exrImage));

  ref_image.ResetAndAlloc(header);

  const ezUInt32 uiPixelCount = header.GetWidth() * header.GetHeight();
  const ezUInt32 uiNumDstChannels = ezImageFormat::GetNumChannels(header.GetImageFormat());
  const ezUInt32 uiNumSrcChannels = exrHeader.num_channels;

  ezUInt32 uiSrcStride = 0;
  switch (exrHeader.pixel_types[0])
  {
    case TINYEXR_PIXELTYPE_FLOAT:
      uiSrcStride = sizeof(float);
      break;

    case TINYEXR_PIXELTYPE_HALF:
      uiSrcStride = sizeof(float) / 2;
      break;

    case TINYEXR_PIXELTYPE_UINT:
      uiSrcStride = sizeof(ezUInt32);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }


  // src and dst element size is always identical, we only copy from float->float, half->half or uint->uint
  // however data is interleaved in dst, but not interleaved in src

  const ezUInt32 uiDstStride = uiSrcStride * uiNumDstChannels;
  ezUInt8* pDstBytes = ref_image.GetBlobPtr<ezUInt8>().GetPtr();

  if (uiNumDstChannels > uiNumSrcChannels)
  {
    // if we have more dst channels, than in the input data, fill everything with white
    ezMemoryUtils::PatternFill(pDstBytes, 0xFF, uiDstStride * uiPixelCount);
  }

  ezUInt32 c = 0;

  if (uiNumSrcChannels >= 4)
  {
    const ezUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 4)
    {
      // copy to alpha
      CopyChannel(pDstBytes + 3 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 3)
  {
    const ezUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 3)
    {
      // copy to blue
      CopyChannel(pDstBytes + 2 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 2)
  {
    const ezUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 2)
    {
      // copy to green
      CopyChannel(pDstBytes + uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 1)
  {
    const ezUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 1)
    {
      // copy to red
      CopyChannel(pDstBytes, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExrFileFormat::WriteImage(ezStreamWriter& ref_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(ref_stream);
  EZ_IGNORE_UNUSED(image);
  EZ_IGNORE_UNUSED(sFileExtension);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

bool ezExrFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("exr");
}

bool ezExrFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  EZ_IGNORE_UNUSED(sExtension);

  return false;
}

#endif



EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);
