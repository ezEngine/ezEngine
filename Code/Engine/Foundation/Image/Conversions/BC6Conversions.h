#pragma once

#include <Foundation/Image/Image.h>

EZ_FOUNDATION_DLL void ezDecompressImageBC6U(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
EZ_FOUNDATION_DLL void ezDecompressImageBC6S(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
