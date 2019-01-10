#pragma once

#include <Foundation/Image/Formats/ImageFileFormat.h>

class EZ_FOUNDATION_DLL ezBmpFileFormat : public ezImageFileFormat
{
public:
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog) const override;
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
