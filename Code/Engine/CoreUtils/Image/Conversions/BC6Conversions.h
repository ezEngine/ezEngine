#pragma once

#include <CoreUtils/Image/Image.h>

EZ_COREUTILS_DLL void ezDecompressImageBC6U(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
EZ_COREUTILS_DLL void ezDecompressImageBC6S(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
