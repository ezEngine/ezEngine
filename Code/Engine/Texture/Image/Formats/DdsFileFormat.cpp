#include <PCH.h>

#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Image.h>

ezDdsFileFormat g_ddsFormat;

struct ezDdsPixelFormat
{
  ezUInt32 m_uiSize;
  ezUInt32 m_uiFlags;
  ezUInt32 m_uiFourCC;
  ezUInt32 m_uiRGBBitCount;
  ezUInt32 m_uiRBitMask;
  ezUInt32 m_uiGBitMask;
  ezUInt32 m_uiBBitMask;
  ezUInt32 m_uiABitMask;
};

struct ezDdsHeader
{
  ezUInt32 m_uiMagic;
  ezUInt32 m_uiSize;
  ezUInt32 m_uiFlags;
  ezUInt32 m_uiHeight;
  ezUInt32 m_uiWidth;
  ezUInt32 m_uiPitchOrLinearSize;
  ezUInt32 m_uiDepth;
  ezUInt32 m_uiMipMapCount;
  ezUInt32 m_uiReserved1[11];
  ezDdsPixelFormat m_ddspf;
  ezUInt32 m_uiCaps;
  ezUInt32 m_uiCaps2;
  ezUInt32 m_uiCaps3;
  ezUInt32 m_uiCaps4;
  ezUInt32 m_uiReserved2;
};

struct ezDdsResourceDimension
{
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct ezDdsResourceMiscFlags
{
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
};

struct ezDdsHeaderDxt10
{
  ezUInt32 m_uiDxgiFormat;
  ezUInt32 m_uiResourceDimension;
  ezUInt32 m_uiMiscFlag;
  ezUInt32 m_uiArraySize;
  ezUInt32 m_uiMiscFlags2;
};

struct ezDdsdFlags
{
  enum Enum
  {
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
  enum Enum
  {
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
  enum Enum
  {
    COMPLEX = 0x000008,
    MIPMAP = 0x400000,
    TEXTURE = 0x001000,
  };
};

struct ezDdsCaps2
{
  enum Enum
  {
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

ezResult ezDdsFileFormat::ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const
{
  ezDdsHeader fileHeader;
  if (stream.ReadBytes(&fileHeader, sizeof(ezDdsHeader)) != sizeof(ezDdsHeader))
  {
    ezLog::Error(pLog, "Failed to read file header.");
    return EZ_FAILURE;
  }

  if (fileHeader.m_uiMagic != ezDdsMagic)
  {
    ezLog::Error(pLog, "The file is not a recognized DDS file.");
    return EZ_FAILURE;
  }

  if (fileHeader.m_uiSize != 124)
  {
    ezLog::Error(pLog, "The file header size {0} doesn't match the expected size of 124.", fileHeader.m_uiSize);
    return EZ_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if (
      (fileHeader.m_uiFlags & ezDdsdFlags::WIDTH) == 0 ||
      (fileHeader.m_uiFlags & ezDdsdFlags::HEIGHT) == 0)
  {
    ezLog::Error(pLog, "The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return EZ_FAILURE;
  }

  if ((fileHeader.m_uiCaps & ezDdsCaps::TEXTURE) == 0)
  {
    ezLog::Error(pLog, "The file header doesn't specify the mandatory TEXTURE flag.");
    return EZ_FAILURE;
  }

  bool bPitch = (fileHeader.m_uiFlags & ezDdsdFlags::PITCH) != 0;

  ezImageHeader imageHeader;
  imageHeader.SetWidth(fileHeader.m_uiWidth);
  imageHeader.SetHeight(fileHeader.m_uiHeight);

  if (fileHeader.m_ddspf.m_uiSize != 32)
  {
    ezLog::Error(pLog, "The pixel format size {0} doesn't match the expected value of 32.", fileHeader.m_ddspf.m_uiSize);
    return EZ_FAILURE;
  }

  ezDdsHeaderDxt10 headerDxt10;

  ezImageFormat::Enum format = ezImageFormat::UNKNOWN;

  // Data format specified in RGBA masks
  if ((fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::ALPHAPIXELS) != 0 ||
      (fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::RGB) != 0 ||
      (fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::ALPHA) != 0)
  {
    format = ezImageFormat::FromPixelMask(
        fileHeader.m_ddspf.m_uiRBitMask, fileHeader.m_ddspf.m_uiGBitMask,
        fileHeader.m_ddspf.m_uiBBitMask, fileHeader.m_ddspf.m_uiABitMask,
        fileHeader.m_ddspf.m_uiRGBBitCount);

    if (format == ezImageFormat::UNKNOWN)
    {
      ezLog::Error(pLog, "The pixel mask specified was not recognized (R: {0}, G: {1}, B: {2}, A: {3}, Bpp: {4}).",
                   ezArgU(fileHeader.m_ddspf.m_uiRBitMask, 1, false, 16),
                   ezArgU(fileHeader.m_ddspf.m_uiGBitMask, 1, false, 16),
                   ezArgU(fileHeader.m_ddspf.m_uiBBitMask, 1, false, 16),
                   ezArgU(fileHeader.m_ddspf.m_uiABitMask, 1, false, 16),
                   fileHeader.m_ddspf.m_uiRGBBitCount);
      return EZ_FAILURE;
    }

    // Verify that the format we found is correct
    if (ezImageFormat::GetBitsPerPixel(format) != fileHeader.m_ddspf.m_uiRGBBitCount)
    {
      ezLog::Error(pLog, "The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
                   fileHeader.m_ddspf.m_uiRGBBitCount, ezImageFormat::GetBitsPerPixel(format), ezImageFormat::GetName(format));
      return EZ_FAILURE;
    }
  }
  else if ((fileHeader.m_ddspf.m_uiFlags & ezDdpfFlags::FOURCC) != 0)
  {
    if (fileHeader.m_ddspf.m_uiFourCC == ezDdsDxt10FourCc)
    {
      if (stream.ReadBytes(&headerDxt10, sizeof(ezDdsHeaderDxt10)) != sizeof(ezDdsHeaderDxt10))
      {
        ezLog::Error(pLog, "Failed to read file header.");
        return EZ_FAILURE;
      }

      format = ezImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);

      if (format == ezImageFormat::UNKNOWN)
      {
        ezLog::Error(pLog, "The DXGI format {0} has no equivalent image format.", headerDxt10.m_uiDxgiFormat);
        return EZ_FAILURE;
      }
    }
    else
    {
      format = ezImageFormatMappings::FromFourCc(fileHeader.m_ddspf.m_uiFourCC);

      if (format == ezImageFormat::UNKNOWN)
      {
        ezLog::Error(pLog, "The FourCC code '{0}{1}{2}{3}' was not recognized.",
                     ezArgC((char)(fileHeader.m_ddspf.m_uiFourCC >> 0)),
                     ezArgC((char)(fileHeader.m_ddspf.m_uiFourCC >> 8)),
                     ezArgC((char)(fileHeader.m_ddspf.m_uiFourCC >> 16)),
                     ezArgC((char)(fileHeader.m_ddspf.m_uiFourCC >> 24)));
        return EZ_FAILURE;
      }
    }
  }
  else
  {
    ezLog::Error(pLog, "The image format is neither specified as a pixel mask nor as a FourCC code.");
    return EZ_FAILURE;
  }

  imageHeader.SetImageFormat(format);

  const bool bHasMipMaps = (fileHeader.m_uiCaps & ezDdsCaps::MIPMAP) != 0;
  const bool bCubeMap = (fileHeader.m_uiCaps2 & ezDdsCaps2::CUBEMAP) != 0;
  const bool bVolume = (fileHeader.m_uiCaps2 & ezDdsCaps2::VOLUME) != 0;


  if (bHasMipMaps)
  {
    imageHeader.SetNumMipLevels(fileHeader.m_uiMipMapCount);
  }

  // Cubemap and volume texture are mutually exclusive
  if (bVolume && bCubeMap)
  {
    ezLog::Error(pLog, "The header specifies both the VOLUME and CUBEMAP flags.");
    return EZ_FAILURE;
  }

  if (bCubeMap)
  {
    imageHeader.SetNumFaces(6);
  }
  else if (bVolume)
  {
    imageHeader.SetDepth(fileHeader.m_uiDepth);
  }

  image.ResetAndAlloc(imageHeader);

  // If pitch is specified, it must match the computed value
  if (bPitch && image.GetRowPitch(0) != fileHeader.m_uiPitchOrLinearSize)
  {
    ezLog::Error(pLog, "The row pitch specified in the header doesn't match the expected pitch.");
    return EZ_FAILURE;
  }

  ezUInt32 uiDataSize = image.GetArrayPtr<void>().GetCount();

  if (stream.ReadBytes(image.GetArrayPtr<void>().GetPtr(), uiDataSize) != uiDataSize)
  {
    ezLog::Error(pLog, "Failed to read image data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezDdsFileFormat::WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const
{
  const ezImageFormat::Enum format = image.GetImageFormat();
  const ezUInt32 uiBpp = ezImageFormat::GetBitsPerPixel(format);

  const ezUInt32 uiNumFaces = image.GetNumFaces();
  const ezUInt32 uiNumMipLevels = image.GetNumMipLevels();
  const ezUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const ezUInt32 uiWidth = image.GetWidth(0);
  const ezUInt32 uiHeight = image.GetHeight(0);
  const ezUInt32 uiDepth = image.GetDepth(0);

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume = uiDepth > 1;
  bool bCubeMap = uiNumFaces > 1;
  bool bArray = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  ezDdsHeader fileHeader;
  ezDdsHeaderDxt10 headerDxt10;

  ezMemoryUtils::ZeroFill(&fileHeader, 1);
  ezMemoryUtils::ZeroFill(&headerDxt10, 1);

  fileHeader.m_uiMagic = ezDdsMagic;
  fileHeader.m_uiSize = 124;
  fileHeader.m_uiWidth = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = ezDdsdFlags::WIDTH | ezDdsdFlags::HEIGHT | ezDdsdFlags::CAPS | ezDdsdFlags::PIXELFORMAT;

  if (bHasMipMaps)
  {
    fileHeader.m_uiFlags |= ezDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if (bVolume)
  {
    // Volume and array are incompatible
    if (bArray)
    {
      ezLog::Error(pLog, "The image is both an array and volume texture. This is not supported.");
      return EZ_FAILURE;
    }

    fileHeader.m_uiFlags |= ezDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  switch (ezImageFormat::GetType(image.GetImageFormat()))
  {
    case ezImageFormatType::LINEAR:
      fileHeader.m_uiFlags |= ezDdsdFlags::PITCH;
      fileHeader.m_uiPitchOrLinearSize = image.GetRowPitch(0);
      break;

    case ezImageFormatType::BLOCK_COMPRESSED:
      fileHeader.m_uiFlags |= ezDdsdFlags::LINEARSIZE;
      fileHeader.m_uiPitchOrLinearSize = 0; /// \todo sub-image size
      break;

    default:
      ezLog::Error(pLog, "Unknown image format type.");
      return EZ_FAILURE;
  }

  fileHeader.m_uiCaps = ezDdsCaps::TEXTURE;

  if (bCubeMap)
  {
    if (uiNumFaces != 6)
    {
      ezLog::Error(pLog, "The image is a cubemap, but has {0} faces instead of the expected 6.", uiNumFaces);
      return EZ_FAILURE;
    }

    if (bVolume)
    {
      ezLog::Error(pLog, "The image is both a cubemap and volume texture. This is not supported.");
      return EZ_FAILURE;
    }

    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= ezDdsCaps2::CUBEMAP |
                            ezDdsCaps2::CUBEMAP_POSITIVEX | ezDdsCaps2::CUBEMAP_NEGATIVEX |
                            ezDdsCaps2::CUBEMAP_POSITIVEY | ezDdsCaps2::CUBEMAP_NEGATIVEY |
                            ezDdsCaps2::CUBEMAP_POSITIVEZ | ezDdsCaps2::CUBEMAP_NEGATIVEZ;
  }

  if (bArray)
  {
    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if (bVolume)
  {
    fileHeader.m_uiCaps |= ezDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= ezDdsCaps2::VOLUME;
  }

  if (bHasMipMaps)
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
  if (!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel masks
    if ((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) && format == ezImageFormat::FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBpp))
    {
      fileHeader.m_ddspf.m_uiFlags = ezDdpfFlags::ALPHAPIXELS | ezDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = ezImageFormat::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if (uiFourCc != 0)
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

  if (bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if (uiDxgiFormat == 0)
    {
      ezLog::Error(pLog, "The image needs to be written as a DXT10 file, but no matching DXGI format was found for '{0}'.",
                   ezImageFormat::GetName(format));
      return EZ_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags = ezDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = ezDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if (bVolume)
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE3D;
    }
    else if (uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = ezDdsResourceDimension::TEXTURE1D;
    }

    if (bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = ezDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if (stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != EZ_SUCCESS)
  {
    ezLog::Error(pLog, "Failed to write image header.");
    return EZ_FAILURE;
  }

  if (bDxt10)
  {
    if (stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != EZ_SUCCESS)
    {
      ezLog::Error(pLog, "Failed to write image DX10 header.");
      return EZ_FAILURE;
    }
  }

  if (stream.WriteBytes(image.GetArrayPtr<void>().GetPtr(), image.GetArrayPtr<void>().GetCount()) != EZ_SUCCESS)
  {
    ezLog::Error(pLog, "Failed to write image data.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

bool ezDdsFileFormat::CanReadFileType(const char* szExtension) const
{
  return ezStringUtils::IsEqual_NoCase(szExtension, "dds");
}

bool ezDdsFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}



EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_DdsFileFormat);

