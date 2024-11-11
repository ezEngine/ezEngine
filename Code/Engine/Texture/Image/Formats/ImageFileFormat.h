#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>

class ezStreamReader;
class ezStreamWriter;
class ezImage;
class ezImageView;
class ezStringBuilder;
class ezImageHeader;

class EZ_TEXTURE_DLL ezImageFileFormat
{
public:
  /// \brief Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual ezResult ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const = 0;

  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given ezLogInterface.
  virtual ezResult ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given ezLogInterface.
  virtual ezResult WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(ezStringView sExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(ezStringView sExtension) const = 0;

  /// \brief Returns an ezImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate ezImageFileFormat.
  static const ezImageFileFormat* GetReaderFormat(ezStringView sExtension);

  /// \brief Returns an ezImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate ezImageFileFormat.
  static const ezImageFileFormat* GetWriterFormat(ezStringView sExtension);

  static ezResult ReadImageHeader(ezStringView sFileName, ezImageHeader& ref_header);
};

/// \brief Base class for a registered (globally known) ezImageFileFormat.
///
/// This is an enumerable class, so all known formats can be retrieved through the ezEnumerable interface.
/// For example:
///
///   for (auto format = ezRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
///   {
///     auto& type = format->GetFormatType();
///   }
class EZ_TEXTURE_DLL ezRegisteredImageFileFormat : public ezEnumerable<ezRegisteredImageFileFormat>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRegisteredImageFileFormat);

public:
  ezRegisteredImageFileFormat();
  ~ezRegisteredImageFileFormat();

  virtual const ezImageFileFormat& GetFormatType() const = 0;
};

/// \brief Template used to automatically register an ezImageFileFormat globally.
///
/// Place a global variable of the desired type in some CPP file to register the type:
///
///   ezImageFileFormatRegistrator<ezDdsFileFormat> g_ddsFormat;
///
/// For the format to be available on platforms that use static linking, you may also need to add
///   EZ_STATICLINK_FORCE
template <class TYPE>
class ezImageFileFormatRegistrator : public ezRegisteredImageFileFormat
{
public:
  virtual const ezImageFileFormat& GetFormatType() const override
  {
    return m_Format;
  }

private:
  TYPE m_Format;
};
