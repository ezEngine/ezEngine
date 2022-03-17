#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class EZ_TEXTURE_DLL ezTgaFileFormat : public ezImageFileFormat
{
public:
  virtual ezResult ReadImageHeader(ezStreamReader& stream, ezImageHeader& header, const char* szFileExtension) const override;
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, const char* szFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
