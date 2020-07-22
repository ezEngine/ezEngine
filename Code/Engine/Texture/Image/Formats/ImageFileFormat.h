#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>

class ezStreamReader;
class ezStreamWriter;
class ezImage;
class ezImageView;
class ezStringBuilder;

class EZ_TEXTURE_DLL ezImageFileFormat : public ezEnumerable<ezImageFileFormat>
{
public:
  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given ezLogInterface.
  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given ezLogInterface.
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(const char* szExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(const char* szExtension) const = 0;

  /// \brief Returns an ezImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate ezImageFileFormat.
  static ezImageFileFormat* GetReaderFormat(const char* extension);

  /// \brief Returns an ezImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate ezImageFileFormat.
  static ezImageFileFormat* GetWriterFormat(const char* extension);

  EZ_DECLARE_ENUMERABLE_CLASS(ezImageFileFormat);
};
