#pragma once

#include <Texture/Image/Image.h>

EZ_TEXTURE_DLL ezColorBaseUB ezDecompressA4B4G4R4(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressB4G4R4A4(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressB5G6R5(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressB5G5R5X1(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressB5G5R5A1(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressX1B5G5R5(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezColorBaseUB ezDecompressA1B5G5R5(ezUInt16 uiColor);
EZ_TEXTURE_DLL ezUInt16 ezCompressA4B4G4R4(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressB4G4R4A4(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressB5G6R5(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressB5G5R5X1(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressB5G5R5A1(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressX1B5G5R5(ezColorBaseUB color);
EZ_TEXTURE_DLL ezUInt16 ezCompressA1B5G5R5(ezColorBaseUB color);
