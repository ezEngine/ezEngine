#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class EZ_TEXTURE_DLL ezTgaFileFormat : public ezImageFileFormat
{
public:
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
