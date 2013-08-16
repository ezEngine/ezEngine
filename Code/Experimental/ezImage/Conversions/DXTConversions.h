#pragma once

#include <Image.h>

#include <ImageDefinitions.h>

void ezDecompressBlockBC1(const ezUInt8* pSource, ezBgra* pTarget, bool bForceFourColorMode);
void ezDecompressBlockBC4(const ezUInt8* pSource, ezUInt8* pTarget, ezUInt32 uiStride);

void ezDecompressImageBC1(const ezImage& source, ezImage& target);
void ezDecompressImageBC2(const ezImage& source, ezImage& target);
void ezDecompressImageBC3(const ezImage& source, ezImage& target);
void ezDecompressImageBC4(const ezImage& source, ezImage& target);
void ezDecompressImageBC5(const ezImage& source, ezImage& target);