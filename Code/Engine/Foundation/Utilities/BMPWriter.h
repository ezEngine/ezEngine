#pragma once

#include <Foundation/Basics.h>

// forward declarations
class ezIBinaryStreamWriter;

/// \brief utility class for writing bmp images
class ezBMPWriter
{
private:
#pragma pack(push, 1)
  struct BMPHeader
  {
    char marker[2];
    ezUInt32 uiFileSize;
    ezUInt32 uiReserved;
    ezUInt32 uiOffsetToData;

    inline BMPHeader() : uiFileSize(0), uiReserved(0) 
    {
      marker[0] = 'B';
      marker[1] = 'M';
    }
  };
#pragma pack(pop)

  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(BMPHeader) == 14, "byte layout of BMPHeader broken");

  struct BitmapCompression
  {
    enum Enum
    {
      RGB = 0
    };
  };

  struct BitmapInfoHeader 
  {
    ezUInt32 uiSizeOfHeader;
    ezInt32 iWidth;
    ezInt32 iHeight;
    ezUInt16 uiNumColorPlanes;
    ezUInt16 uiBitsPerPixel;
    ezUInt32 uiCompressionMethod;
    ezUInt32 uiDataSize; // size of the raw image data
    ezInt32 iResolutionX; // pixel per meter
    ezInt32 iResolutionY; // pixel per meter
    ezUInt32 uiNumColorsInPalette;
    ezUInt32 uiNumColorsUsed;

    inline BitmapInfoHeader() : 
    uiSizeOfHeader(sizeof(BitmapInfoHeader)),
      uiNumColorPlanes(1),
      uiCompressionMethod(BitmapCompression::RGB),
      iResolutionX(2835), //72 pixels per inch
      iResolutionY(2835), //72 pixels per inch
      uiNumColorsInPalette(0),
      uiNumColorsUsed(0)         
    {
    };
  };

  EZ_CHECK_AT_COMPILETIME_MSG(sizeof(BitmapInfoHeader) == 40, "byte layout of BitmapInfoHeader broken");

public:

  struct DataFormat
  {
    enum Enum
    {
      Grayscale8,  // 8-bit per channel grayscale image
      RGB8        // 8-bit per cannel rgb data
    };
  };

  /// \brief writes image data as bmp file to a out stream
  ///
  /// \param data
  ///   the data to write. The size is specified by uiWidth and uiHeight and the format by format
  ///
  /// \param uiWidth
  ///   the width of the data
  ///
  /// \param uiHeight
  ///   the height of the data

  EZ_FOUNDATION_DLL static ezResult Write(ezArrayPtr<const ezUInt8> data, ezUInt32 uiWidth, ezUInt32 uiHeight, DataFormat::Enum format, ezIBinaryStreamWriter& outStream);
};