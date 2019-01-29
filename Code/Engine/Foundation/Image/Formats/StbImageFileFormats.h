#pragma once

#include <Foundation/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class EZ_FOUNDATION_DLL ezStbImageFileFormats : public ezImageFileFormat
{
public:
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
