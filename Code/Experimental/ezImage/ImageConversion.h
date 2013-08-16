#pragma once

#include <Image.h>

/// \brief Helper class containing utilities to convert between different image formats and layouts.
class ezImageConversion
{
public:
  /// \brief Prototype for a function converting an image to a different format.
  typedef void(*ConversionFunction)(const ezImage& source, ezImage& target);

  /// \brief Converts an image to the format and alignment of the target image.
  ///
  /// \returns EZ_FAILURE if no suitable conversion function is known.
  static ezResult Convert(const ezImage& source, ezImage& target);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  static ezImageFormat::Enum
    FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum* pCompatibleFormats, ezUInt32 uiNumCompatible);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  template<int N> static ezImageFormat::Enum
    FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum(&compatibleFormats)[N])
  {
    return FindClosestCompatibleFormat(format, compatibleFormats, N);
  }

  /// \brief Registers a function which allows conversion between to image formats.
  static void RegisterConversionFunction(ConversionFunction pFunction, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat);


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, Image);

  static void Startup();
  static void Shutdown();
};