#pragma once

#include <ezImage.h>

class ezImageConversion
{
public:
  typedef void(*ConversionFunction)(const ezImage& source, ezImage& target);

  template<int N> static ezImageFormat::Enum
    FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum(&compatibleFormats)[N])
  {
    return FindClosestCompatibleFormat(format, compatibleFormats, N);
  }

  static ezImageFormat::Enum
    FindClosestCompatibleFormat(ezImageFormat::Enum format, const ezImageFormat::Enum* pCompatibleFormats, ezUInt32 uiNumCompatible);

  static ezResult Convert(const ezImage& source, ezImage& target);

  static void RegisterConversionFunction(ConversionFunction pFunction, ezImageFormat::Enum sourceFormat, ezImageFormat::Enum targetFormat);

  static void RebuildConversionTable();

  static void Startup();

  static void Shutdown();
};