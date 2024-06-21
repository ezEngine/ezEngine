#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_TINYEXR_SUPPORT

/// EXR file format support using TinyEXR.
class EZ_TEXTURE_DLL ezExrFileFormat : public ezImageFileFormat
{
public:
  ezResult ReadImageHeader(ezStreamReader& ref_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const override;
  ezResult ReadImage(ezStreamReader& ref_stream, ezImage& ref_image, ezStringView sFileExtension) const override;
  ezResult WriteImage(ezStreamWriter& ref_stream, const ezImageView& image, ezStringView sFileExtension) const override;

  bool CanReadFileType(ezStringView sExtension) const override;
  bool CanWriteFileType(ezStringView sExtension) const override;
};

#endif
