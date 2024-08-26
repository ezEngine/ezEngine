#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/TgaFileFormat.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

// EZ_STATICLINK_FORCE
ezTgaFileFormat g_TgaFormat;

struct TgaImageDescriptor
{
  ezUInt8 m_iAlphaBits : 4;
  ezUInt8 m_bFlipH : 1;
  ezUInt8 m_bFlipV : 1;
  ezUInt8 m_Ignored : 2;
};

// see Wikipedia for details:
// http://de.wikipedia.org/wiki/Targa_Image_File
struct TgaHeader
{
  ezInt8 m_iImageIDLength;
  ezInt8 m_Ignored1;
  ezInt8 m_ImageType;
  ezInt8 m_Ignored2[9];
  ezInt16 m_iImageWidth;
  ezInt16 m_iImageHeight;
  ezInt8 m_iBitsPerPixel;
  TgaImageDescriptor m_ImageDescriptor;
};

static_assert(sizeof(TgaHeader) == 18);


static inline ezColorLinearUB GetPixelColor(const ezImageView& image, ezUInt32 x, ezUInt32 y, const ezUInt32 uiHeight)
{
  ezColorLinearUB c(255, 255, 255, 255);

  const ezUInt8* pPixel = image.GetPixelPointer<ezUInt8>(0, 0, 0, x, uiHeight - y - 1, 0);

  switch (image.GetImageFormat())
  {
    case ezImageFormat::R8G8B8A8_UNORM:
      c.r = pPixel[0];
      c.g = pPixel[1];
      c.b = pPixel[2];
      c.a = pPixel[3];
      break;
    case ezImageFormat::B8G8R8A8_UNORM:
      c.a = pPixel[3];
      // fall through
    case ezImageFormat::B8G8R8_UNORM:
    case ezImageFormat::B8G8R8X8_UNORM:
      c.r = pPixel[2];
      c.g = pPixel[1];
      c.b = pPixel[0];
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return c;
}


ezResult ezTgaFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  ezImageFormat::Enum compatibleFormats[] = {
    ezImageFormat::R8G8B8A8_UNORM,
    ezImageFormat::B8G8R8A8_UNORM,
    ezImageFormat::B8G8R8X8_UNORM,
    ezImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("No conversion from format '{0}' to a format suitable for TGA files known.", ezImageFormat::GetName(image.GetImageFormat()));
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

  const bool bCompress = true;

  // Write the header
  {
    ezUInt8 uiHeader[18];
    ezMemoryUtils::ZeroFill(uiHeader, 18);

    if (!bCompress)
    {
      // uncompressed TGA
      uiHeader[2] = 2;
    }
    else
    {
      // compressed TGA
      uiHeader[2] = 10;
    }

    uiHeader[13] = static_cast<ezUInt8>(image.GetWidth(0) / 256);
    uiHeader[15] = static_cast<ezUInt8>(image.GetHeight(0) / 256);
    uiHeader[12] = static_cast<ezUInt8>(image.GetWidth(0) % 256);
    uiHeader[14] = static_cast<ezUInt8>(image.GetHeight(0) % 256);
    uiHeader[16] = static_cast<ezUInt8>(ezImageFormat::GetBitsPerPixel(image.GetImageFormat()));

    inout_stream.WriteBytes(uiHeader, 18).IgnoreResult();
  }

  const bool bAlpha = image.GetImageFormat() != ezImageFormat::B8G8R8_UNORM;

  const ezUInt32 uiWidth = image.GetWidth(0);
  const ezUInt32 uiHeight = image.GetHeight(0);

  if (!bCompress)
  {
    // Write image uncompressed

    for (ezUInt32 y = 0; y < uiWidth; ++y)
    {
      for (ezUInt32 x = 0; x < uiHeight; ++x)
      {
        const ezColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        inout_stream << c.b;
        inout_stream << c.g;
        inout_stream << c.r;

        if (bAlpha)
          inout_stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    ezInt32 iRLE = 0;

    ezColorLinearUB pc = {};
    ezStaticArray<ezColorLinearUB, 129> unequal;
    ezInt32 iEqual = 0;

    for (ezUInt32 y = 0; y < uiHeight; ++y)
    {
      for (ezUInt32 x = 0; x < uiWidth; ++x)
      {
        const ezColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        if (iRLE == 0) // no comparison possible yet
        {
          pc = c;
          iRLE = 1;
          unequal.PushBack(c);
        }
        else if (iRLE == 1) // has one value gathered for comparison
        {
          if (c == pc)
          {
            iRLE = 2;   // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc = c;   // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            ezUInt8 uiRepeat = static_cast<ezUInt8>(iEqual + 127);

            inout_stream << uiRepeat;
            inout_stream << pc.b;
            inout_stream << pc.g;
            inout_stream << pc.r;

            if (bAlpha)
              inout_stream << pc.a;

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 3)
        {
          if ((c != pc) && (unequal.GetCount() < 128))
          {
            unequal.PushBack(c);
            pc = c;
          }
          else
          {
            ezUInt8 uiRepeat = (unsigned char)(unequal.GetCount()) - 1;
            inout_stream << uiRepeat;

            for (ezUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              inout_stream << unequal[i].b;
              inout_stream << unequal[i].g;
              inout_stream << unequal[i].r;

              if (bAlpha)
                inout_stream << unequal[i].a;
            }

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
      }
    }


    if (iRLE == 1) // has one value gathered for comparison
    {
      ezUInt8 uiRepeat = 0;

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      ezUInt8 uiRepeat = static_cast<ezUInt8>(iEqual + 127);

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 3)
    {
      ezUInt8 uiRepeat = (ezUInt8)(unequal.GetCount()) - 1;
      inout_stream << uiRepeat;

      for (ezUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        inout_stream << unequal[i].b;
        inout_stream << unequal[i].g;
        inout_stream << unequal[i].r;

        if (bAlpha)
          inout_stream << unequal[i].a;
      }
    }
  }

  return EZ_SUCCESS;
}


static ezResult ReadBytesChecked(ezStreamReader& inout_stream, void* pDest, ezUInt32 uiNumBytes)
{
  if (inout_stream.ReadBytes(pDest, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

template <typename TYPE>
static ezResult ReadBytesChecked(ezStreamReader& inout_stream, TYPE& ref_dest)
{
  return ReadBytesChecked(inout_stream, &ref_dest, sizeof(TYPE));
}

static ezResult ReadImageHeaderImpl(ezStreamReader& inout_stream, ezImageHeader& ref_header, TgaHeader& ref_tgaHeader)
{
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageIDLength));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_Ignored1));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageType));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, &ref_tgaHeader.m_Ignored2, 9));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageWidth));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iImageHeight));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_iBitsPerPixel));
  EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, ref_tgaHeader.m_ImageDescriptor));

  // ignore optional data
  if (inout_stream.SkipBytes(ref_tgaHeader.m_iImageIDLength) != ref_tgaHeader.m_iImageIDLength)
    return EZ_FAILURE;

  const ezUInt32 uiBytesPerPixel = ref_tgaHeader.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((ref_tgaHeader.m_iImageWidth <= 0) || (ref_tgaHeader.m_iImageHeight <= 0) || ((uiBytesPerPixel != 1) && (uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (ref_tgaHeader.m_ImageType != 2 && ref_tgaHeader.m_ImageType != 3 && ref_tgaHeader.m_ImageType != 10 && ref_tgaHeader.m_ImageType != 11))
  {
    ezLog::Error("TGA has an invalid header: Width = {0}, Height = {1}, BPP = {2}, ImageType = {3}", ref_tgaHeader.m_iImageWidth, ref_tgaHeader.m_iImageHeight, ref_tgaHeader.m_iBitsPerPixel, ref_tgaHeader.m_ImageType);
    return EZ_FAILURE;
  }

  // Set image data

  if (uiBytesPerPixel == 1)
    ref_header.SetImageFormat(ezImageFormat::R8_UNORM);
  else if (uiBytesPerPixel == 3)
    ref_header.SetImageFormat(ezImageFormat::B8G8R8_UNORM);
  else
    ref_header.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);

  ref_header.SetWidth(ref_tgaHeader.m_iImageWidth);
  ref_header.SetHeight(ref_tgaHeader.m_iImageHeight);
  ref_header.SetDepth(1);

  return EZ_SUCCESS;
}

ezResult ezTgaFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  EZ_PROFILE_SCOPE("ezTgaFileFormat::ReadImageHeader");

  TgaHeader tgaHeader;
  return ReadImageHeaderImpl(inout_stream, ref_header, tgaHeader);
}

ezResult ezTgaFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);

  EZ_PROFILE_SCOPE("ezTgaFileFormat::ReadImage");

  ezImageHeader imageHeader;
  TgaHeader tgaHeader;
  EZ_SUCCEED_OR_RETURN(ReadImageHeaderImpl(inout_stream, imageHeader, tgaHeader));

  const ezUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  ref_image.ResetAndAlloc(imageHeader);

  if (tgaHeader.m_ImageType == 3)
  {
    // uncompressed greyscale

    const ezUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (ezInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (ezInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (ezInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else if (tgaHeader.m_ImageType == 2)
  {
    // uncompressed

    const ezUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (ezInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (ezInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (ezInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else
  {
    // compressed

    ezInt32 iCurrentPixel = 0;
    const int iPixelCount = tgaHeader.m_iImageWidth * tgaHeader.m_iImageHeight;

    do
    {
      ezUInt8 uiChunkHeader = 0;

      EZ_SUCCEED_OR_RETURN(ReadBytesChecked(inout_stream, uiChunkHeader));

      const ezInt32 numToRead = (uiChunkHeader & 127) + 1;

      if (iCurrentPixel + numToRead > iPixelCount)
      {
        ezLog::Error("TGA contents are invalid");
        return EZ_FAILURE;
      }

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        // Read RAW color values
        for (ezInt32 i = 0; i < numToRead; ++i)
        {
          const ezInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const ezInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, col, row, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        ezUInt8 uiBuffer[4] = {255, 255, 255, 255};

        // read the current color
        inout_stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated
        for (ezInt32 i = 0; i < numToRead; ++i)
        {
          const ezInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const ezInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          ezUInt8* pPixel = ref_image.GetPixelPointer<ezUInt8>(0, 0, 0, col, row, 0);

          // BGR
          pPixel[0] = uiBuffer[0];

          if (uiBytesPerPixel > 1)
          {
            pPixel[1] = uiBuffer[1];
            pPixel[2] = uiBuffer[2];

            // Alpha
            if (uiBytesPerPixel == 4)
              pPixel[3] = uiBuffer[3];
          }

          ++iCurrentPixel;
        }
      }
    } while (iCurrentPixel < iPixelCount);
  }

  return EZ_SUCCESS;
}

bool ezTgaFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("tga");
}

bool ezTgaFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  return CanReadFileType(sExtension);
}



EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_TgaFileFormat);
