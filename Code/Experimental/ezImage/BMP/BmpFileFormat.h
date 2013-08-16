#pragma once

#include <ImageFileFormat.h>

class ezBmpFileFormat : public ezIImageFileFormat
{
public:
  virtual ezResult ReadImage(ezIBinaryStreamReader& stream, ezImage& image, ezStringBuilder& errorOut) const EZ_OVERRIDE;
  virtual ezResult WriteImage(ezIBinaryStreamWriter& stream, const ezImage& image, ezStringBuilder& errorOut) const EZ_OVERRIDE;

  virtual bool IsKnownExtension(const char* szExtension) const EZ_OVERRIDE;
};
