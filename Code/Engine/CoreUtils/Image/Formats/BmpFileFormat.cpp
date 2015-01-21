#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Formats/BmpFileFormat.h>

#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>

ezBmpFileFormat g_bmpFormat;

enum ezBmpCompression
{
  RGB = 0L,
  RLE8 = 1L,
  RLE4 = 2L,
  BITFIELDS = 3L,
  JPEG = 4L,
  PNG = 5L,
};


#pragma pack(push,1)
struct ezBmpFileHeader
{
  ezUInt16 m_type;
  ezUInt32 m_size;
  ezUInt16 m_reserved1;
  ezUInt16 m_reserved2;
  ezUInt32 m_offBits;
};
#pragma pack(pop)

static const char* ezBmpFileHeaderFormat = "wdwwd";

struct ezBmpFileInfoHeader {
  ezUInt32			m_size;
  ezUInt32			m_width;
  ezUInt32			m_height;
  ezUInt16			m_planes;
  ezUInt16			m_bitCount;
  ezBmpCompression	m_compression;
  ezUInt32			m_sizeImage;
  ezUInt32			m_xPelsPerMeter;
  ezUInt32			m_yPelsPerMeter;
  ezUInt32			m_clrUsed;
  ezUInt32			m_clrImportant;
};

static const char* ezBmpFileInfoHeaderFormat = "dddwwdddddd";

struct ezCIEXYZ
{
  int ciexyzX;
  int ciexyzY;
  int ciexyzZ;
};

struct ezCIEXYZTRIPLE
{
  ezCIEXYZ ciexyzRed;
  ezCIEXYZ ciexyzGreen;
  ezCIEXYZ ciexyzBlue;
};

struct ezBmpFileInfoHeaderV4 {
  ezUInt32        m_redMask;
  ezUInt32        m_greenMask;
  ezUInt32        m_blueMask;
  ezUInt32        m_alphaMask;
  ezUInt32        m_csType;
  ezCIEXYZTRIPLE  m_endpoints;
  ezUInt32        m_gammaRed;
  ezUInt32        m_gammaGreen;
  ezUInt32        m_gammaBlue;
};

EZ_CHECK_AT_COMPILETIME(sizeof(ezCIEXYZTRIPLE) == 3 * 3 * 4);

// just to be on the safe side
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezCIEXYZTRIPLE) == sizeof(CIEXYZTRIPLE));
#endif

static const char* ezBmpFileInfoHeaderV4Format = "dddwwdddddd";

struct ezBmpFileInfoHeaderV5 {
  ezUInt32        m_intent;
  ezUInt32        m_profileData;
  ezUInt32        m_profileSize;
  ezUInt32        m_reserved;
};

static const ezUInt16 ezBmpFileMagic = 0x4D42u;

struct ezBmpBgrxQuad {
  EZ_DECLARE_POD_TYPE();

  ezBmpBgrxQuad()
  {
  }

  ezBmpBgrxQuad(ezUInt8 red, ezUInt8 green, ezUInt8 blue) : m_blue(blue), m_green(green), m_red(red), m_reserved(0)
  {
  }

  ezUInt8 m_blue;
  ezUInt8 m_green;
  ezUInt8 m_red;
  ezUInt8 m_reserved;
};

ezResult ezBmpFileFormat::WriteImage(ezStreamWriterBase& stream, const ezImage& image, ezLogInterface* pLog) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  ezImageFormat::Enum compatibleFormats[] =
  {
    ezImageFormat::B8G8R8X8_UNORM,
    ezImageFormat::B8G8R8A8_UNORM,
    ezImageFormat::B8G8R8_UNORM,
    ezImageFormat::B5G5R5X1_UNORM,
    ezImageFormat::B5G6R5_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversionBase::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(pLog, "No conversion from format '%s' to a format suitable for BMP files known.", ezImageFormat::GetName(image.GetImageFormat()));
    return EZ_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    ezImage convertedImage;
    if (ezImageConversionBase::Convert(image, convertedImage, format) != EZ_SUCCESS)
    {
      // This should never happen
      EZ_ASSERT_DEV(false, "ezImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return EZ_FAILURE;
    }

    return WriteImage(stream, convertedImage, pLog);
  }
  
  ezUInt32 uiRowPitch = image.GetRowPitch(0);

  ezUInt32 uiHeight = image.GetHeight(0);

  int dataSize = uiRowPitch * uiHeight;

  ezBmpFileInfoHeader fileInfoHeader;
  fileInfoHeader.m_width = image.GetWidth(0);
  fileInfoHeader.m_height = uiHeight;
  fileInfoHeader.m_planes = 1;
  fileInfoHeader.m_bitCount = ezImageFormat::GetBitsPerPixel(format);

  fileInfoHeader.m_sizeImage = 0; // Can be zero unless we store the data compressed

  fileInfoHeader.m_xPelsPerMeter = 0;
  fileInfoHeader.m_yPelsPerMeter = 0;
  fileInfoHeader.m_clrUsed = 0;
  fileInfoHeader.m_clrImportant = 0;

  bool bWriteColorMask = false;

  // Prefer to write a V3 header
  ezUInt32 uiHeaderVersion = 3;

  switch (format)
  {
  case ezImageFormat::B8G8R8X8_UNORM:
  case ezImageFormat::B5G5R5X1_UNORM:
  case ezImageFormat::B8G8R8_UNORM:
    fileInfoHeader.m_compression = RGB;
    break;

  case ezImageFormat::B8G8R8A8_UNORM:
    fileInfoHeader.m_compression = BITFIELDS;
    uiHeaderVersion = 4;
    break;

  case ezImageFormat::B5G6R5_UNORM:
    fileInfoHeader.m_compression = BITFIELDS;
    bWriteColorMask = true;
    break;

  default:
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(!bWriteColorMask || uiHeaderVersion <= 3, "Internal bug");

  ezUInt32 uiFileInfoHeaderSize = sizeof(ezBmpFileInfoHeader);
  ezUInt32 uiHeaderSize = sizeof(ezBmpFileHeader);

  if (uiHeaderVersion >= 4)
  {
    uiFileInfoHeaderSize += sizeof(ezBmpFileInfoHeaderV4);
  }
  else if (bWriteColorMask)
  {
    uiHeaderSize += 3 * sizeof(ezUInt32);
  }

  uiHeaderSize += uiFileInfoHeaderSize;

  fileInfoHeader.m_size = uiFileInfoHeaderSize;

  ezBmpFileHeader header;
  header.m_type = ezBmpFileMagic;
  header.m_size = uiHeaderSize + dataSize;
  header.m_reserved1 = 0;
  header.m_reserved2 = 0;
  header.m_offBits = uiHeaderSize;


  const void* dataPtr = image.GetDataPointer<void>();
  
  // Write all data
  if (stream.WriteBytes(&header, sizeof(header)) != EZ_SUCCESS)
  {
    ezLog::Error(pLog, "Failed to write header.");
    return EZ_FAILURE;
  }

  if (stream.WriteBytes(&fileInfoHeader, sizeof(fileInfoHeader)) != EZ_SUCCESS)
  {
    ezLog::Error(pLog, "Failed to write fileInfoHeader.");
    return EZ_FAILURE;
  }

  if (uiHeaderVersion >= 4)
  {
    ezBmpFileInfoHeaderV4 fileInfoHeaderV4;
    memset(&fileInfoHeaderV4, 0, sizeof(fileInfoHeaderV4));

    fileInfoHeaderV4.m_redMask = ezImageFormat::GetRedMask(format);
    fileInfoHeaderV4.m_greenMask = ezImageFormat::GetGreenMask(format);
    fileInfoHeaderV4.m_blueMask = ezImageFormat::GetBlueMask(format);
    fileInfoHeaderV4.m_alphaMask = ezImageFormat::GetAlphaMask(format);

    if (stream.WriteBytes(&fileInfoHeaderV4, sizeof(fileInfoHeaderV4)) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write fileInfoHeaderV4.");
      return EZ_FAILURE;
    }
  }
  else if (bWriteColorMask)
  {
    struct
    {
      ezUInt32 m_red;
      ezUInt32 m_green;
      ezUInt32 m_blue;
    } colorMask;


    colorMask.m_red = ezImageFormat::GetRedMask(format);
    colorMask.m_green = ezImageFormat::GetGreenMask(format);
    colorMask.m_blue = ezImageFormat::GetBlueMask(format);

    if (stream.WriteBytes(&colorMask, sizeof(colorMask)) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write colorMask.");
      return EZ_FAILURE;
    }
  }

  const ezUInt32 uiPaddedRowPitch = ((uiRowPitch - 1) / 4 + 1) * 4;
  // Write rows in reverse order
  for (ezInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
  {
    if (stream.WriteBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiPaddedRowPitch) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write data.");
      return EZ_FAILURE;
    }
  }
  
  return EZ_SUCCESS;
}

namespace
{
  ezUInt32 ExtractBits(const void* pData, ezUInt32 uiBitAddress, ezUInt32 uiNumBits)
  {
    ezUInt32 uiMask = (1U << uiNumBits) - 1;
    ezUInt32 uiByteAddress = uiBitAddress / 8;
    ezUInt32 uiShiftAmount = 7 - (uiBitAddress % 8 + uiNumBits - 1);

    return (reinterpret_cast<const ezUInt8*>(pData)[uiByteAddress] >> uiShiftAmount) & uiMask;
  }
}

ezResult ezBmpFileFormat::ReadImage(ezStreamReaderBase& stream, ezImage& image, ezLogInterface* pLog) const
{
  ezBmpFileHeader fileHeader;
  if (stream.ReadBytes(&fileHeader, sizeof(ezBmpFileHeader)) != sizeof(ezBmpFileHeader))
  {
    ezLog::Error(pLog, "Failed to read header data.");
    return EZ_FAILURE;
  }

  // Some very old BMP variants may have different magic numbers, but we don't support them.
  if (fileHeader.m_type != ezBmpFileMagic)
  {
    ezLog::Error(pLog, "The file is not a recognized BMP file.");
    return EZ_FAILURE;
  }

  // We expect at least header version 3
  ezUInt32 uiHeaderVersion = 3;
  ezBmpFileInfoHeader fileInfoHeader;
  if (stream.ReadBytes(&fileInfoHeader, sizeof(ezBmpFileInfoHeader)) != sizeof(ezBmpFileInfoHeader))
  {
    ezLog::Error(pLog, "Failed to read header data (V3).");
    return EZ_FAILURE;
  }

  int remainingHeaderBytes = fileInfoHeader.m_size - sizeof(fileInfoHeader);

  // File header shorter than expected - happens with corrupt files or e.g. with OS/2 BMP files which may have shorter headers
  if (remainingHeaderBytes < 0)
  {
    ezLog::Error(pLog, "The file header was shorter than expected.");
    return EZ_FAILURE;
  }

  // Newer files may have a header version 4 (required for transparency)
  ezBmpFileInfoHeaderV4 fileInfoHeaderV4;
  if (remainingHeaderBytes >= sizeof(ezBmpFileInfoHeaderV4))
  {
    uiHeaderVersion = 4;
    if (stream.ReadBytes(&fileInfoHeaderV4, sizeof(ezBmpFileInfoHeaderV4)) != sizeof(ezBmpFileInfoHeaderV4))
    {
      ezLog::Error(pLog, "Failed to read header data (V4).");
      return EZ_FAILURE;
    }
    remainingHeaderBytes -= sizeof(ezBmpFileInfoHeaderV4);
  }

  // Skip rest of header
  if (stream.SkipBytes(remainingHeaderBytes) != remainingHeaderBytes)
  {
    ezLog::Error(pLog, "Failed to skip remaining header data.");
    return EZ_FAILURE;
  }

  ezUInt32 uiBpp = fileInfoHeader.m_bitCount;

  // Find target format to load the image
  ezImageFormat::Enum format = ezImageFormat::UNKNOWN;
  bool bIndexed = false;
  bool bCompressed = false;

  switch (fileInfoHeader.m_compression)
  {
    // RGB or indexed data
  case  RGB:
    switch (uiBpp)
    {
    case 1:
    case 4:
    case 8:
      bIndexed = true;

      // We always decompress indexed to BGRX, since the palette is specified in this format
      format = ezImageFormat::B8G8R8X8_UNORM;
      break;

    case 16:
      format = ezImageFormat::B5G5R5X1_UNORM;
      break;

    case 24:
      format = ezImageFormat::B8G8R8_UNORM;
      break;
      
    case 32:
      format = ezImageFormat::B8G8R8X8_UNORM;
    }
    break;

    // RGB data, but with the color masks specified in place of the palette
  case BITFIELDS:
    switch (uiBpp)
    {
    case 16:
    case 32:
      // In case of old headers, the color masks appear after the header (and aren't counted as part of it)
      if (uiHeaderVersion < 4)
      {
        // Color masks (w/o alpha channel)
        struct
        {
          ezUInt32 m_red;
          ezUInt32 m_green;
          ezUInt32 m_blue;
        } colorMask;

        if (stream.ReadBytes(&colorMask, sizeof(colorMask)) != sizeof(colorMask))
        {
          return EZ_FAILURE;
        }

        format = ezImageFormat::FromPixelMask(colorMask.m_red, colorMask.m_green, colorMask.m_blue, 0);
      }
      else
      {
        // For header version four and higher, the color masks are part of the header
        format = ezImageFormat::FromPixelMask(
          fileInfoHeaderV4.m_redMask, fileInfoHeaderV4.m_greenMask,
          fileInfoHeaderV4.m_blueMask, fileInfoHeaderV4.m_alphaMask);
      }
   
      break;
    }
    break;

  case RLE4:
    if (uiBpp == 4)
    {
      bIndexed = true;
      bCompressed = true;
      format = ezImageFormat::B8G8R8X8_UNORM;
    }
    break;

  case RLE8:
    if (uiBpp == 8)
    {
      bIndexed = true;
      bCompressed = true;
      format = ezImageFormat::B8G8R8X8_UNORM;
    }
    break;
  }

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(pLog, "Unknown or unsupported BMP encoding.");
    return EZ_FAILURE;
  }

  const ezUInt32 uiWidth = fileInfoHeader.m_width;

  if (uiWidth > 65536)
  {
    ezLog::Error(pLog, "Image specifies width > 65536. Header corrupted?");
    return EZ_FAILURE;
  }

  const ezUInt32 uiHeight = fileInfoHeader.m_height;

  if (uiHeight > 65536)
  {
    ezLog::Error(pLog, "Image specifies height > 65536. Header corrupted?");
    return EZ_FAILURE;
  }

  ezUInt32 uiDataSize = fileInfoHeader.m_sizeImage;

  if (uiDataSize > 1024 * 1024 * 1024)
  {
    ezLog::Error(pLog, "Image specifies data size > 1GiB. Header corrupted?");
    return EZ_FAILURE;
  }

  int uiRowPitchIn = (uiWidth * uiBpp + 31) / 32 * 4;

  if (uiDataSize == 0)
  {
    if (fileInfoHeader.m_compression != RGB)
    {
      ezLog::Error(pLog, "The data size wasn't specified in the header.");
      return EZ_FAILURE;
    }
    uiDataSize = uiRowPitchIn * uiHeight;
  }

  // Set image data
  image.SetImageFormat(format);
  image.SetNumMipLevels(1);
  image.SetNumArrayIndices(1);
  image.SetNumFaces(1);

  image.SetWidth(uiWidth);
  image.SetHeight(uiHeight);
  image.SetDepth(1);

  image.AllocateImageData();

  ezUInt32 uiRowPitch = image.GetRowPitch(0);

  if (bIndexed)
  {
    // If no palette size was specified, the full available palette size will be used
    ezUInt32 paletteSize = fileInfoHeader.m_clrUsed;
    if (paletteSize == 0)
    {
      paletteSize = 1U << uiBpp;
    }
    else if(paletteSize > 65536)
    {
      ezLog::Error(pLog, "Palette size > 65536.");
      return EZ_FAILURE;
    }

    ezDynamicArray<ezBmpBgrxQuad> palette;
    palette.SetCount(paletteSize);
    if (stream.ReadBytes(&palette[0], paletteSize * sizeof(ezBmpBgrxQuad)) != paletteSize * sizeof(ezBmpBgrxQuad))
    {
      ezLog::Error(pLog, "Failed to read palette data.");
      return EZ_FAILURE;
    }

    if (bCompressed)
    {
      // Compressed data is always in pairs of bytes
      if (uiDataSize % 2 != 0)
      {
        ezLog::Error(pLog, "The data size is not a multiple of 2 bytes in an RLE-compressed file.");
        return EZ_FAILURE;
      }

      ezDynamicArray<ezUInt8> compressedData;
      compressedData.SetCount(uiDataSize);
      
      if (stream.ReadBytes(&compressedData[0], uiDataSize) != uiDataSize)
      {
        ezLog::Error(pLog, "Failed to read data.");
        return EZ_FAILURE;
      }

      const ezUInt8* pIn = &compressedData[0];
      const ezUInt8* pInEnd = pIn + uiDataSize;

      // Current output position
      ezUInt32 uiRow = uiHeight - 1;
      ezUInt32 uiCol = 0;

      ezBmpBgrxQuad* pLine = image.GetPixelPointer<ezBmpBgrxQuad>(0, 0, 0, 0, uiRow, 0);

      // Decode RLE data directly to RGBX
      while(pIn < pInEnd)
      {
        ezUInt32 uiByte1 = *pIn++;
        ezUInt32 uiByte2 = *pIn++;

        // Relative mode - the first byte specified a number of indices to be repeated, the second one the indices
        if (uiByte1 > 0)
        {
          // Clamp number of repetitions to row width.
          // The spec isn't clear on this point, but some files pad the number of encoded indices for some reason.
          uiByte1 = ezMath::Min(uiByte1, uiWidth - uiCol);

          if (uiBpp == 4)
          {
            // Alternate between two indices.
            for (ezUInt32 uiRep = 0; uiRep < uiByte1 / 2; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
              pLine[uiCol++] = palette[uiByte2 & 0x0F];
            }

            // Repeat the first index for odd numbers of repetitions.
            if (uiByte1 & 1)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
            }
          }
          else /* if (uiBpp == 8) */
          {
            // Repeat a single index.
            for (ezUInt32 uiRep = 0; uiRep < uiByte1; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2];
            }
          }
        }
        else
        {
          // Absolute mode - the first byte specifies a number of indices encoded separately, or is a special marker
          switch (uiByte2)
          {
          // End of line marker
          case 0:
            {

              // Fill up with palette entry 0
              while(uiCol < uiWidth)
              {
                pLine[uiCol++] = palette[0];
              }

              // Begin next line
              uiCol = 0;
              uiRow--;
              pLine -= uiWidth;
            }
            
            break;

          // End of image marker
          case 1:
            // Check that we really reached the end of the image.
            if (uiRow != 0 && uiCol != uiHeight - 1)
            {
              ezLog::Error(pLog, "Unexpected end of image marker found.");
              return EZ_FAILURE;
            }
            break;

          case 2:
            ezLog::Error(pLog, "Found a RLE compression position delta - this is not supported.");
            return EZ_FAILURE;

          default:
            // Read uiByte2 number of indices
            
            // More data than fits into the image or can be read?
            if (uiCol + uiByte2 > uiWidth || pIn + (uiByte2 + 1) / 2 > pInEnd)
            {
              return EZ_FAILURE;
            }

            if (uiBpp == 4)
            {
              for (ezUInt32 uiRep = 0; uiRep < uiByte2 / 2; uiRep++)
              {
                ezUInt32 uiIndices = *pIn++;
                pLine[uiCol++] = palette[uiIndices >> 4];
                pLine[uiCol++] = palette[uiIndices & 0x0f];
              }

              if (uiByte2 & 1)
              {
                pLine[uiCol++] = palette[*pIn++ >> 4];
              }

              // Pad to word boundary
              pIn += (uiByte2 / 2 + uiByte2) & 1;
            }
            else /* if (uiBpp == 8) */
            {
              for (ezUInt32 uiRep = 0; uiRep < uiByte2; uiRep++)
              {
                pLine[uiCol++] = palette[*pIn++];
              }

              // Pad to word boundary
              pIn += uiByte2 & 1;
            }
          }
        }
      }
    }
    else
    {
      ezDynamicArray<ezUInt8> indexedData;
      indexedData.SetCount(uiDataSize);
      if (stream.ReadBytes(&indexedData[0], uiDataSize) != uiDataSize)
      {
        ezLog::Error(pLog, "Failed to read data.");
        return EZ_FAILURE;
      }

      // Convert to non-indexed
      for (ezUInt32 uiRow = 0; uiRow < uiHeight; uiRow++)
      {
        ezUInt8* pIn = &indexedData[uiRowPitchIn * uiRow];

        // Convert flipped vertically
        ezBmpBgrxQuad* pOut = image.GetPixelPointer<ezBmpBgrxQuad>(0, 0, 0, 0, uiHeight - uiRow - 1, 0);
        for (ezUInt32 uiCol = 0; uiCol < image.GetWidth(0); uiCol++)
        {
          ezUInt32 uiIndex = ExtractBits(pIn, uiCol * uiBpp, uiBpp);
          if (uiIndex >= palette.GetCount())
          {
            ezLog::Error(pLog, "Image contains invalid palette indices.");
            return EZ_FAILURE;
          }
          pOut[uiCol] = palette[uiIndex];
        }
      }
    }
  }
  else
  {
    // Format must match the number of bits in the file
    if (ezImageFormat::GetBitsPerPixel(format) != uiBpp)
    {
      ezLog::Error(pLog, "The number of bits per pixel specified in the file (%d) does not match the expected value of %d for the format '%s'.",
        uiBpp, ezImageFormat::GetBitsPerPixel(format), ezImageFormat::GetName(format));
      return EZ_FAILURE;
    }

    // Skip palette data. Having a palette here doesn't make sense, but is not explicitly disallowed by the standard.
    ezUInt32 paletteSize = fileInfoHeader.m_clrUsed * sizeof(ezBmpBgrxQuad);
    if (stream.SkipBytes(paletteSize) != paletteSize)
    {
      ezLog::Error(pLog, "Failed to skip palette data.");
      return EZ_FAILURE;
    }

    // Read rows in reverse order
    for (ezInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
    {
      if (stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != uiRowPitch)
      {
        ezLog::Error(pLog, "Failed to read row data.");
        return EZ_FAILURE;
      }
      if (stream.SkipBytes(uiRowPitchIn - uiRowPitch) != uiRowPitchIn - uiRowPitch)
      {
        ezLog::Error(pLog, "Failed to skip row data.");
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

bool ezBmpFileFormat::CanReadFileType(const char* szExtension) const
{
  return
    ezStringUtils::IsEqual_NoCase(szExtension, "bmp") ||
    ezStringUtils::IsEqual_NoCase(szExtension, "dib") ||
    ezStringUtils::IsEqual_NoCase(szExtension, "rle");
}

bool ezBmpFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}


EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Formats_BmpFileFormat);

