#pragma once

#include <Image.h>

void ezDecompressImageBC6U(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
void ezDecompressImageBC6S(const ezImage& source, ezImage& target, ezImageFormat::Enum targetFormat);
