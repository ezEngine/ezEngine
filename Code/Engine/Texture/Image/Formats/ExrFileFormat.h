#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class EZ_TEXTURE_DLL ezExrFileFormat : public ezImageFileFormat
{
public:
  ezResult ReadImageHeader(ezStreamReader& ref_stream, ezImageHeader& ref_header, const char* szFileExtension) const override;
  ezResult ReadImage(ezStreamReader& ref_stream, ezImage& ref_image, const char* szFileExtension) const override;
  ezResult WriteImage(ezStreamWriter& ref_stream, const ezImageView& image, const char* szFileExtension) const override;

  bool CanReadFileType(const char* szExtension) const override;
  bool CanWriteFileType(const char* szExtension) const override;
};

#endif
