#pragma once

#include <ImageFileFormat.h>

class ezDdsFormat : public ezImageFileFormat
{
public:
  virtual ezResult readImage(ezIBinaryStreamReader& stream, ezImage& image, ezStringBuilder& errorOut) const EZ_OVERRIDE;

  virtual ezResult writeImage(ezIBinaryStreamWriter& stream, const ezImage& image, ezStringBuilder& errorOut) const EZ_OVERRIDE;
};
