#pragma once

#include <Image.h>
#include <ImageDefinitions.h>

ezBgra ezDecompress565(ezUInt16 uiColor);

void ezConvertImage4444_8888(const ezImage& source, ezImage& target);
void ezConvertImageF32_U8(const ezImage& source, ezImage& target);
