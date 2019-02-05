#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct ezTexConvChannelMapping
{
  ezInt8 m_iInputImageIndex = -1;
  ezTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct ezTexConvSliceChannelMapping
{
  ezTexConvChannelMapping m_Channel[4] = {
    ezTexConvChannelMapping{-1, ezTexConvChannelValue::Red},
    ezTexConvChannelMapping{-1, ezTexConvChannelValue::Green},
    ezTexConvChannelMapping{-1, ezTexConvChannelValue::Blue},
    ezTexConvChannelMapping{-1, ezTexConvChannelValue::Alpha},
  };
};

class EZ_TEXTURE_DLL ezTexConvDesc
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvDesc);

public:
  ezTexConvDesc() = default;

  ezHybridArray<ezString, 4> m_InputFiles;
  ezDynamicArray<ezImage> m_InputImages;

  ezHybridArray<ezTexConvSliceChannelMapping, 6> m_ChannelMappings;

  // output type / platform
  ezEnum<ezTexConvOutputType> m_OutputType;         // TODO: implement cubemaps, 3D, decal atlas
  ezEnum<ezTexConvTargetPlatform> m_TargetPlatform; // TODO: implement android

  // low resolution output
  ezUInt32 m_uiLowResMipmaps = 0;

  // thumbnail output
  ezUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  ezEnum<ezTexConvUsage> m_Usage;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  ezUInt32 m_uiMinResolution = 16;
  ezUInt32 m_uiMaxResolution = 1024 * 8;
  ezUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  ezEnum<ezTexConvFilterMode> m_FilterMode; // only used when writing to ez specific formats
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;
  bool m_bPreserveMipmapCoverage = false;
  float m_fMipmapAlphaThreshold = 0.5f;

  // Misc options
  bool m_bFlipHorizontal = false;
  bool m_bPremultiplyAlpha = false;
  float m_fHdrExposureBias = 0.0f;

  // ez specific
  ezUInt64 m_uiAssetHash = 0;
  ezUInt16 m_uiAssetVersion = 0;

  // Decals
  ezString m_sDecalAtlasDescFile;
};
