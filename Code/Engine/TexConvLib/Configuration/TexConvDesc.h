#pragma once

#include <TexConvLib/Basics.h>
#include <TexConvLib/Configuration/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

struct ezTexConvChannelMapping
{
  ezInt8 m_iInputImageIndex = -1;
  ezTexConvChannelValue::Enum m_ChannelValue;
};

class EZ_TEXCONV_DLL ezTexConvDesc
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvDesc);
public:
  ezTexConvDesc() = default;

  ezHybridArray<ezString, 4> m_InputFiles;
  ezDynamicArray<ezImage> m_InputImages;

  /// For 2D textures only: Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
  /// output file The four elements of the array represent the four channels of the output image
  ezTexConvChannelMapping m_Texture2DChannelMapping[4] = {
      ezTexConvChannelMapping{0, ezTexConvChannelValue::Red},
      ezTexConvChannelMapping{0, ezTexConvChannelValue::Green},
      ezTexConvChannelMapping{0, ezTexConvChannelValue::Blue},
      ezTexConvChannelMapping{0, ezTexConvChannelValue::Alpha},
  };

  // output type / platform
  ezEnum<ezTexConvOutputType> m_OutputType;
  ezEnum<ezTexConvTargetPlatform> m_TargetPlatform;

  // low resolution output
  ezUInt32 m_uiLowResOutputResolution = 0;

  // thumbnail output
  ezUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  ezEnum<ezTexConvTargetFormat> m_TargetFormat;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  ezUInt32 m_uiMinResolution = 16;
  ezUInt32 m_uiMaxResolution = 1024 * 8;
  ezUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  ezEnum<ezTexConvFilterMode> m_FilterMode;
  ezEnum<ezTexConvWrapMode> m_WrapModes[3]; // U, V, W
  bool m_bPreserveMipmapCoverage = false;
  float m_fMipmapAlphaThreshold = 0.5f;

  // Misc options
  bool m_bFlipHorizontal = false;
  bool m_bPremultiplyAlpha = false;
  float m_fHdrExposureBias = 1.0f;

  // ez specific
  ezUInt64 m_uiAssetHash = 0;
  ezUInt16 m_uiAssetVersion = 0;
};
