#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Utilities/EnumerableClass.h>

class ezStreamReaderBase;
class ezStreamWriterBase;
class ezImage;
class ezStringBuilder;

class ezIImageFileFormat : public ezEnumerable<ezIImageFileFormat>
{
public:
  virtual ezResult ReadImage(ezStreamReaderBase& stream, ezImage& image, ezStringBuilder& errorOut) const = 0;
  virtual ezResult WriteImage(ezStreamWriterBase& stream, const ezImage& image, ezStringBuilder& errorOut) const = 0;

  virtual bool IsKnownExtension(const char* szExtension) const = 0;

  EZ_DECLARE_ENUMERABLE_CLASS(ezIImageFileFormat);
};
