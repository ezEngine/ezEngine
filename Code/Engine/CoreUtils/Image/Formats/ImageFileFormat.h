#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Logging/Log.h>

class ezStreamReader;
class ezStreamWriter;
class ezImage;
class ezStringBuilder;

class EZ_COREUTILS_DLL ezImageFileFormat : public ezEnumerable<ezImageFileFormat>
{
public:
  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given ezLogInterface.
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given ezLogInterface.
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImage& image, ezLogInterface* pLog) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType (const char* szExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(const char* szExtension) const = 0;


  EZ_DECLARE_ENUMERABLE_CLASS(ezImageFileFormat);
};
