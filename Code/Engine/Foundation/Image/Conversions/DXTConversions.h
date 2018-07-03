#pragma once

#include <Foundation/Image/Image.h>

EZ_FOUNDATION_DLL void ezDecompressBlockBC1(const ezUInt8* pSource, ezColorLinearUB* pTarget, bool bForceFourColorMode);
EZ_FOUNDATION_DLL void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride);
