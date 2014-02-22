#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Logging/Log.h>

class ezStreamReaderBase;
class ezStreamWriterBase;
class ezImage;
class ezStringBuilder;

class EZ_COREUTILS_DLL ezImageFileFormatBase : public ezEnumerable<ezImageFileFormatBase>
{
public:
  virtual ezResult ReadImage(ezStreamReaderBase& stream, ezImage& image, ezLogInterface* pLog) const = 0;
  virtual ezResult WriteImage(ezStreamWriterBase& stream, const ezImage& image, ezLogInterface* pLog) const = 0;

  virtual bool IsKnownExtension(const char* szExtension) const = 0;

  EZ_DECLARE_ENUMERABLE_CLASS(ezImageFileFormatBase);
};
