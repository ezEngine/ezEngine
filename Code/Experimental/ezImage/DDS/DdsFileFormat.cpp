#include <DDS/DdsFileFormat.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/IBinaryStream.h>

#include <Image.h>
#include <ImageConversion.h>
#include <ImageFormatMappings.h>

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

struct ezDdsResourceDimension {
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct ezDdsResourceMiscFlags {
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
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

      format = ezImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);
    }
    else
    {
      format = ezImageFormatMappings::FromFourCc(fileHeader.m_ddspf.m_uiFourCC);
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
  if(bPitch && image.GetRowPitch(0) != fileHeader.m_uiPitchOrLinearSize)
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
  const ezImageFormat::Enum format = image.GetImageFormat();

  const ezUInt32 uiNumFaces = image.GetNumFaces();
  const ezUInt32 uiNumMipLevels = image.GetNumMipLevels();
  const ezUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const ezUInt32 uiWidth = image.GetWidth(0);
  const ezUInt32 uiHeight = image.GetHeight(0);
  const ezUInt32 uiDepth = image.GetDepth(0);
  
  if(image.GetRowAlignment() != 1 || image.GetDepthAlignment() != 1 || image.GetSubImageAlignment() != 1)
  {
    // Copy into new image with default alignment (which matches the DDS layout)
    ezImage converted;

    converted.SetWidth(uiWidth);
    converted.SetHeight(uiHeight);
    converted.SetDepth(uiDepth);
    converted.SetNumMipLevels(uiNumMipLevels);
    converted.SetNumFaces(uiNumFaces);
    converted.SetNumArrayIndices(uiNumArrayIndices);

    converted.SetImageFormat(format);

    if(ezImageConversion::Convert(image, converted) != EZ_SUCCESS)
    {
      return EZ_FAILURE;
    }

    return writeImage(stream, converted);
  }

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume = uiDepth > 1;
  bool bCubeMap = uiNumFaces > 1;
  bool bArray = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  ezDdsHeader fileHeader;
  ezDdsHeaderDxt10 headerDxt10;

  ezMemoryUtils::ZeroFill(&fileHeader);
  ezMemoryUtils::ZeroFill(&headerDxt10);

  fileHeader.m_uiMagic = ezDdsMagic;
  fileHeader.m_uiSize = 124;
  fileHeader.m_uiWidth = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = ezDdsdFlags::WIDTH | ezDdsdFlags::HEIGHT | ezDdsdFlags::CAPS | ezDdsdFlags::PIXELFORMAT;

  if(bHasMipMaps)
  {
    fileHeader.m_uiFlags |= ezDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if(bVolume)
  {
    // Volume and array are incompatible
    if(bArray)
    {
      return EZ_FAILURE;
    }

    fileHeader.m_uiFlags |= ezDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  switch(ezImageFormat::GetType(image.GetImageFormat()))
  {
  case ezImageFormatType::LINEAR:
    fileHeader.m_uiFlags |= ezDdsdFlags::PITCH;
    fileHeader.m_uiPitchOrLinearSize = image.GetRowPitch(0);
    break;

  case ezImageFormatType::BLOCK_COMPRESSED:
    fileHeader.m_uiFlags |= ezDdsdFlags::LINEARSIZE;
    fileHeader.m_uiPitchOrLinearSize = 0;     // TODO: subimage size
    break;

  default:
    return EZ_FAILURE;
  }

  fileHeader.m_uiCaps = ezDdsCaps::TEXTURE;

  if(bCubeMap)
  {
    // Anything other than 6 faces or using a volume texture at the same time doesn't make sense
    if(uiNumFaces != 6 || bVolume)
    {
      return EZ_FAILURE;
    }

    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= ezDdsCaps2::CUBEMAP |
      ezDdsCaps2::CUBEMAP_POSITIVEX | ezDdsCaps2::CUBEMAP_NEGATIVEX |
      ezDdsCaps2::CUBEMAP_POSITIVEY | ezDdsCaps2::CUBEMAP_NEGATIVEY |
      ezDdsCaps2::CUBEMAP_POSITIVEZ | ezDdsCaps2::CUBEMAP_NEGATIVEZ;
  }

  if(bArray)
  {
    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if(bVolume)
  {
    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= ezDdsCaps2::VOLUME;
  }

  if(bHasMipMaps)
  {
    fileHeader.m_uiCaps |= ezDdsCaps::MIPMAP | ezDdsCaps::COMPLEX;
  }

  fileHeader.m_ddspf.m_uiSize = 32;

  ezUInt32 uiRedMask = ezImageFormat::GetRedMask(format);
  ezUInt32 uiGreenMask = ezImageFormat::GetGreenMask(format);
  ezUInt32 uiBlueMask = ezImageFormat::GetBlueMask(format);
  ezUInt32 uiAlphaMask = ezImageFormat::GetAlphaMask(format);

  ezUInt32 uiFourCc = ezImageFormatMappings::ToFourCc(format);
  ezUInt32 uiDxgiFormat = ezImageFormatMappings::ToDxgiFormat(format);

  // When not required to use a DXT10 texture, try to write a legacy DDS by specifying FourCC or pixel masks
  if(!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel masks
    if((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) && format == ezImageFormat::FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask))
    {
      fileHeader.m_ddspf.m_uiFlags = ezDdpfFlags::ALPHAPIXELS | ezDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = ezImageFormat::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if(uiFourCc != 0)
    {
      fileHeader.m_ddspf.m_uiFlags = ezDdpfFlags::FOURCC;
      fileHeader.m_ddspf.m_uiFourCC = uiFourCc;
    }
    else
    {
      // Fallback to DXT10 path
      bDxt10 = true;
    }
  }

  if(bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if(uiDxgiFormat == 0)
    {
      return EZ_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags = ezDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = ezDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if(bVolume)
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE3D;
    }
    else if(uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE1D;
    }

    if(bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = ezDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if(stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != EZ_SUCCESS)
  {
    return EZ_FAILURE;
  }

  if(!bDxt10 || stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != EZ_SUCCESS)
  {
    return EZ_FAILURE;
  }

  if(stream.WriteBytes(image.GetDataPointer<void>(), image.GetDataSize()) != EZ_SUCCESS)
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
