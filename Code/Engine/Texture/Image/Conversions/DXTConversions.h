#pragma once

#include <Texture/Image/Image.h>

class ezColorLinear16f;

EZ_TEXTURE_DLL void ezDecompressBlockBC1(const ezUInt8* pSource, ezColorBaseUB* pTarget, bool bForceFourColorMode);
EZ_TEXTURE_DLL void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride, ezUInt8 bias);
EZ_TEXTURE_DLL void ezDecompressBlockBC6(const ezUInt8* pSource, ezColorLinear16f* pTarget, bool isSigned);
EZ_TEXTURE_DLL void ezDecompressBlockBC7(const ezUInt8* pSource, ezColorBaseUB* pTarget);

EZ_TEXTURE_DLL void ezUnpackPaletteBC4(ezUInt32 a0, ezUInt32 a1, ezUInt32* alphas);
