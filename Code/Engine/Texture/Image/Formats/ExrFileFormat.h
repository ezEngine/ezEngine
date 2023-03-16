#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class EZ_TEXTURE_DLL ezExrFileFormat : public ezImageFileFormat
{
public:
  ezResult ReadImageHeader(ezStreamReader& stream, ezImageHeader& header, const char* szFileExtension) const override;
  ezResult ReadImage(ezStreamReader& stream, ezImage& image, const char* szFileExtension) const override;
  ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, const char* szFileExtension) const override;

  bool CanReadFileType(const char* szExtension) const override;
  bool CanWriteFileType(const char* szExtension) const override;
};

#endif
