#include <DDS/ezDdsFileFormat.h>

#include "Foundation/Containers/DynamicArray.h"
#include "ezImage.h"
#include "ezImageConversion.h"
#include "Foundation/IO/IBinaryStream.h"
#include "ezFileFormatMappings.h"

struct ezDdsPixelFormat {
  ezUInt32 m_uiSize;
  ezUInt32 m_uiFlags;
  ezUInt32 m_uiFourCC;
  ezUInt32 m_uiRGBBitCount;
  ezUInt32 m_uiRBitMask;
  ezUInt32 m_uiGBitMask;
  ezUInt32 m_uiBBitMask;
  ezUInt32 m_uiABitMask;
};

struct ezDdsHeader {
  ezUInt32         m_uiMagic;
  ezUInt32         m_uiSize;
  ezUInt32         m_uiFlags;
  ezUInt32         m_uiHeight;
  ezUInt32         m_uiWidth;
  ezUInt32         m_uiPitchOrLinearSize;
  ezUInt32         m_uiDepth;
  ezUInt32         m_uiMipMapCount;
  ezUInt32         m_uiReserved1[11];
  ezDdsPixelFormat m_ddspf;
  ezUInt32         m_uiCaps;
  ezUInt32         m_uiCaps2;
  ezUInt32         m_uiCaps3;
  ezUInt32         m_uiCaps4;
  ezUInt32         m_uiReserved2;
};


struct ezDdsHeaderDxt10 {
  ezUInt32 m_uiDxgiFormat;
  ezUInt32 m_uiResourceDimension;
  ezUInt32 m_uiMiscFlag;
  ezUInt32 m_uiArraySize;
  ezUInt32 m_uiMiscFlags2;
};

struct ezDdsdFlags
{
  enum Enum {
    CAPS = 0x000001,
    HEIGHT = 0x000002,
    WIDTH = 0x000004,
    PITCH = 0x000008,
    PIXELFORMAT = 0x001000,
    MIPMAPCOUNT = 0x020000,
    LINEARSIZE = 0x080000,
    DEPTH = 0x800000,
  };
};

struct ezDdpfFlags
{
  enum Enum {
    ALPHAPIXELS = 0x00001,
    ALPHA = 0x00002,
    FOURCC = 0x00004,
    RGB = 0x00040,
   YUV = 0x00200,
   LUMINANCE = 0x20000,
  };
};

struct ezDdsCaps
{
  enum Enum {
    COMPLEX = 0x000008,
    MIPMAP = 0x400000,
    TEXTURE = 0x001000, 
  };
};

struct ezDdsCaps2 
{
  enum Enum {
    CUBEMAP = 0x000200,
    CUBEMAP_POSITIVEX = 0x000400,
    CUBEMAP_NEGATIVEX = 0x000800,
    CUBEMAP_POSITIVEY = 0x001000,
    CUBEMAP_NEGATIVEY = 0x002000,
    CUBEMAP_POSITIVEZ = 0x004000,
    CUBEMAP_NEGATIVEZ = 0x008000,
    VOLUME = 0x200000,
  };
};

static const ezUInt32 ezDdsMagic = 0x20534444;
static const ezUInt32 ezDdsDxt10FourCc = 0x30315844;

ezResult ezDdsFormat::readImage(ezIBinaryStreamReader& stream, ezImage& image) const
{
  ezDdsHeader fileHeader;
  if(stream.ReadBytes(&fileHeader, sizeof(ezDdsHeader)) != sizeof(ezDdsHeader))
  {
    return EZ_FAILURE;
  }

  if(fileHeader.m_uiMagic != ezDdsMagic)
  {
    return EZ_FAILURE;
  }

  if(fileHeader.m_uiSize != 124)
  {
    return EZ_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if(
    (fileHeader.m_uiFlags & ezDdsdFlags::WIDTH) == 0 ||
    (fileHeader.m_uiFlags & ezDdsdFlags::HEIGHT) == 0)
  {
    return EZ_FAILURE;
  }
  
  bool bPitch = (fileHeader.m_uiFlags & ezDdsdFlags::PITCH) != 0;

  image.SetWidth(fileHeader.m_uiWidth);
  image.SetHeight(fileHeader.m_uiHeight);
 
  if(fileHeader.m_ddspf.m_uiSize != 32)
  {
    return EZ_FAILURE;
  }

  bool bDxt10 = false;
  ezDdsHeaderDxt10 headerDxt10;

  ezImageFormat::Enum format = ezImageFormat::UNKNOWN;

  // Data format specified in RGBA masks
  if((fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::ALPHAPIXELS) != 0 || (fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::RGB) != 0)
  {
    format = ezImageFormat::FromPixelMask(
      fileHeader.m_ddspf.m_uiRBitMask, fileHeader.m_ddspf.m_uiGBitMask,
      fileHeader.m_ddspf.m_uiBBitMask, fileHeader.m_ddspf.m_uiABitMask);

    // Verify that the format we found is correct
    if(ezImageFormat::GetBitsPerPixel(format) != fileHeader.m_ddspf.m_uiRGBBitCount)
    {
      return EZ_FAILURE;
    }
  }
  else if((fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::FOURCC) != 0)
  { 
    if(fileHeader.m_ddspf.m_uiFourCC == ezDdsDxt10FourCc)
    {
      if(stream.ReadBytes(&headerDxt10, sizeof(ezDdsHeaderDxt10)) != sizeof(ezDdsHeaderDxt10))
      {
        return EZ_FAILURE;
      }
      bDxt10 = true;

      format = ezFileFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);
    }
    else
    {
      format = ezImageFormat::FromFourCc(fileHeader.m_ddspf.m_uiFourCC);
    }
  }

  if(format == ezImageFormat::UNKNOWN)
  {
    return EZ_FAILURE;
  }

  image.SetImageFormat(format);

  if((fileHeader.m_uiCaps & ezDdsCaps::TEXTURE) == 0)
  {
    return EZ_FAILURE;
  }

  bool bComplex = (fileHeader.m_uiCaps & ezDdsCaps::COMPLEX) != 0;
  bool bHasMipMaps = (fileHeader.m_uiCaps & ezDdsCaps::MIPMAP) != 0;
  bool bCubeMap = (fileHeader.m_uiCaps2 & ezDdsCaps2::CUBEMAP) != 0;
  bool bVolume = (fileHeader.m_uiCaps2 & ezDdsCaps2::VOLUME) != 0;

  // Complex flag must match cubemap or volume flag
  if(bComplex != (bCubeMap || bVolume || bHasMipMaps))
  {
    return EZ_FAILURE;
  }

  if(bHasMipMaps)
  {
    image.SetNumMipLevels(fileHeader.m_uiMipMapCount);
  }

  // Cubemap and volume texture are mutually exclusive
  if(bVolume && bCubeMap)
  {
    return EZ_FAILURE;
  }

  if(bCubeMap)
  {
    image.SetNumFaces(6);
  }
  else if(bVolume)
  {
    image.SetDepth(fileHeader.m_uiDepth);
  }

  image.AllocateImageData();

  // If pitch is specified, it must match the computed value
  if(bPitch && image.GetRowPitch(0, 0, 0) != fileHeader.m_uiPitchOrLinearSize)
  {
    return EZ_FAILURE;
  }

  ezUInt32 uiDataSize = image.GetDataSize();

  if(stream.ReadBytes(image.GetDataPointer<void>(), uiDataSize) != uiDataSize)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezDdsFormat::writeImage(ezIBinaryStreamWriter& stream, const ezImage& image) const
{
  return EZ_FAILURE;
}
