#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Reads Adobe CUBE LUT files (.cube) as 3D images.
///
/// Only 3D LUTs are supported, files that specify a 1D LUT (LUT_1D_SIZE) fail to load.
/// The result is a volume image with width, height and depth equal to LUT_3D_SIZE in R32G32B32A32_FLOAT format,
/// where x maps to the red, y to the green and z to the blue input coordinate. Alpha is always 1.
/// The LUT values are stored as they are in the file, DOMAIN_MIN and DOMAIN_MAX are validated but not applied.
/// Writing is not supported.
class EZ_TEXTURE_DLL ezCubeLutFileFormat : public ezImageFileFormat
{
public:
  virtual ezResult ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const override;
  virtual ezResult ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const override;

  virtual bool CanReadFileType(ezStringView sExtension) const override;
  virtual bool CanWriteFileType(ezStringView sExtension) const override;
};
