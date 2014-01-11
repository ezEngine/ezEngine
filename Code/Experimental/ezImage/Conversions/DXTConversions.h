#pragma once

#include <Image.h>

#include <ImageDefinitions.h>

void ezDecompressBlockBC1(const ezUInt8* pSource, ezBgra* pTarget, bool bForceFourColorMode);
void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride);
