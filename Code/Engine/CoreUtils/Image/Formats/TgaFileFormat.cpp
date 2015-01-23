#include <CoreUtils/PCH.h>
#include <CoreUtils/Image/Formats/TgaFileFormat.h>

#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>

ezTgaFileFormat g_TgaFormat;

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
  ezInt8 m_Ignored3;
};

EZ_CHECK_AT_COMPILETIME(sizeof(TgaHeader) == 18);


static inline ezColorLinearUB GetPixelColor(const ezImage& image, ezUInt32 x, ezUInt32 y, const ezUInt32 uiHeight)
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
  }

  return c;
}


ezResult ezTgaFileFormat::WriteImage(ezStreamWriterBase& stream, const ezImage& image, ezLogInterface* pLog) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  ezImageFormat::Enum compatibleFormats[] =
  {
    ezImageFormat::R8G8B8A8_UNORM,
    ezImageFormat::B8G8R8A8_UNORM,
    ezImageFormat::B8G8R8X8_UNORM,
    ezImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  ezImageFormat::Enum format = ezImageConversionBase::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == ezImageFormat::UNKNOWN)
  {
    ezLog::Error(pLog, "No conversion from format '%s' to a format suitable for TGA files known.", ezImageFormat::GetName(image.GetImageFormat()));
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

    uiHeader[13] = image.GetWidth(0) / 256;
    uiHeader[15] = image.GetHeight(0) / 256;
    uiHeader[12] = image.GetWidth(0) % 256;
    uiHeader[14] = image.GetHeight(0) % 256;
    uiHeader[16] = ezImageFormat::GetBitsPerPixel(image.GetImageFormat());

    stream.WriteBytes(uiHeader, 18);
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

        stream << c.b;
        stream << c.g;
        stream << c.r;

        if (bAlpha)
          stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    ezInt32 iRLE = 0;

    ezColorLinearUB pc;
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
            iRLE = 2; // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc = c; // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            ezUInt8 uiRepeat = iEqual + 127;

            stream << uiRepeat;
            stream << pc.b;
            stream << pc.g;
            stream << pc.r;

            if (bAlpha)
              stream << pc.a;

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
            ezUInt8 uiRepeat = (unsigned char) (unequal.GetCount()) - 1;
            stream << uiRepeat;

            for (ezUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              stream << unequal[i].b;
              stream << unequal[i].g;
              stream << unequal[i].r;

              if (bAlpha)
                stream << unequal[i].a;
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

      stream << uiRepeat;
      stream << pc.b;
      stream << pc.g;
      stream << pc.r;

      if (bAlpha)
        stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      ezUInt8 uiRepeat = iEqual + 127;

      stream << uiRepeat;
      stream << pc.b;
      stream << pc.g;
      stream << pc.r;

      if (bAlpha)
        stream << pc.a;
    }
    else if (iRLE == 3)
    {
      ezUInt8 uiRepeat = (ezUInt8) (unequal.GetCount()) - 1;
      stream << uiRepeat;

      for (ezUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        stream << unequal[i].b;
        stream << unequal[i].g;
        stream << unequal[i].r;

        if (bAlpha)
          stream << unequal[i].a;
      }
    }
  }

  return EZ_SUCCESS;
}


ezResult ezTgaFileFormat::ReadImage(ezStreamReaderBase& stream, ezImage& image, ezLogInterface* pLog) const
{
  TgaHeader Header;
  stream >> Header.m_iImageIDLength;
  stream >> Header.m_Ignored1;
  stream >> Header.m_ImageType;
  stream.ReadBytes(&Header.m_Ignored2, 9);
  stream >> Header.m_iImageWidth;
  stream >> Header.m_iImageHeight;
  stream >> Header.m_iBitsPerPixel;
  stream >> Header.m_Ignored3;

  // ignore optional data
  stream.SkipBytes(Header.m_iImageIDLength);

  const ezUInt32 uiBytesPerPixel = Header.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((Header.m_iImageWidth <= 0) || (Header.m_iImageHeight <= 0) || ((uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (Header.m_ImageType != 2 && Header.m_ImageType != 10))
  {
    ezLog::Error(pLog, "TGA has an invalid header: Width = %i, Height = %i, BPP = %i, ImageType = %i", Header.m_iImageWidth, Header.m_iImageHeight, Header.m_iBitsPerPixel, Header.m_ImageType);
    return EZ_FAILURE;
  }

  // Set image data
  if (uiBytesPerPixel == 3)
    image.SetImageFormat(ezImageFormat::B8G8R8_UNORM);
  else
    image.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM);

  image.SetNumMipLevels(1);
  image.SetNumArrayIndices(1);
  image.SetNumFaces(1);

  image.SetWidth(Header.m_iImageWidth);
  image.SetHeight(Header.m_iImageHeight);
  image.SetDepth(1);

  image.AllocateImageData();

  ezUInt32 uiRowPitch = image.GetRowPitch(0);



  if (Header.m_ImageType == 2)
  {
    // uncompressed

    const ezUInt32 uiBytesPerRow = uiBytesPerPixel * Header.m_iImageWidth;

    // read each row (gets rid of the row pitch
    for (ezInt32 y = 0; y < Header.m_iImageHeight; ++y)
      stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, 0, Header.m_iImageHeight - y - 1, 0), uiBytesPerRow);
  }
  else
  {
    // compressed

    ezInt32 iCurrentPixel = 0;
    const int iPixelCount = Header.m_iImageWidth * Header.m_iImageHeight;

    do
    {
      ezUInt8 uiChunkHeader = 0;

      stream >> uiChunkHeader;

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        uiChunkHeader++;

        // Read RAW color values
        for (ezInt32 i = 0; i < (ezInt32) uiChunkHeader; ++i)
        {
          const ezInt32 x = iCurrentPixel % Header.m_iImageWidth;
          const ezInt32 y = iCurrentPixel / Header.m_iImageWidth;

          stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, x, Header.m_iImageHeight - y - 1, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        uiChunkHeader -= 127; // Subtract 127 to get rid of the ID bit

        ezUInt8 uiBuffer[4] = { 255, 255, 255, 255 };

        // read the current color
        stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated 
        for (ezInt32 i = 0; i < (ezInt32) uiChunkHeader; ++i)
        {
          const ezInt32 x = iCurrentPixel % Header.m_iImageWidth;
          const ezInt32 y = iCurrentPixel / Header.m_iImageWidth;

          ezUInt8* pPixel = image.GetPixelPointer<ezUInt8>(0, 0, 0, x, Header.m_iImageHeight - y - 1, 0);

          // BGR
          pPixel[0] = uiBuffer[0];
          pPixel[1] = uiBuffer[1];
          pPixel[2] = uiBuffer[2];

          // Alpha
          if (uiBytesPerPixel == 4)
            pPixel[3] = uiBuffer[3];

          ++iCurrentPixel;
        }
      }
    }
    while (iCurrentPixel < iPixelCount);
  }

  return EZ_SUCCESS;
}

bool ezTgaFileFormat::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "tga");
}

bool ezTgaFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}


EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Image_Formats_TgaFileFormat);

