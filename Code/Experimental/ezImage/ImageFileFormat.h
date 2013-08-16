#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class ezIBinaryStreamReader;
class ezIBinaryStreamWriter;
class ezImage;
class ezStringBuilder;

class ezIImageFileFormat : public ezEnumerable<ezIImageFileFormat>
{
public:
  virtual ezResult ReadImage(ezIBinaryStreamReader& stream, ezImage& image, ezStringBuilder& errorOut) const = 0;
  virtual ezResult WriteImage(ezIBinaryStreamWriter& stream, const ezImage& image, ezStringBuilder& errorOut) const = 0;

  virtual bool IsKnownExtension(const char* szExtension) const = 0;

  EZ_DECLARE_ENUMERABLE_CLASS(ezIImageFileFormat);
};
