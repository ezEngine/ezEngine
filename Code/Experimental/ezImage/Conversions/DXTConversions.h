#pragma once

#include <Image.h>

void ezDecompressBlockBC1(const ezUInt8* pSource, ezColorBgra8UNorm* pTarget, bool bForceFourColorMode);
void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride);
