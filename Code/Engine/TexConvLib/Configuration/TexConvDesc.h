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

struct EZ_TEXCONV_DLL ezTexConvDesc
{
  ezHybridArray<ezString, 4> m_InputFiles;
  ezDynamicArray<ezImage*> m_InputImages; // TODO: why does this not work with ezImages in the array??

  /// For 2D textures only: Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
  /// output file The four elements of the array represent the four channels of the output image
  ezTexConvChannelMapping m_Texture2DChannelMapping[4] = {
      ezTexConvChannelMapping{-1, ezTexConvChannelValue::Red},
      ezTexConvChannelMapping{-1, ezTexConvChannelValue::Green},
      ezTexConvChannelMapping{-1, ezTexConvChannelValue::Blue},
      ezTexConvChannelMapping{-1, ezTexConvChannelValue::Alpha},
  };

  // output file / type
  ezString m_sOutputFile;
  ezEnum<ezTexConvOutputType> m_OutputType;
  ezEnum<ezTexConvTargetPlatform> m_TargetPlatform;

  // low resolution output
  ezString m_sLowResOutputFile;
  ezUInt32 m_uiLowResOutputResolution = 128;

  // thumbnail output
  ezString m_sThumbnailOutputFile;
  ezUInt32 m_uiThumbnailResolution = 256;

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
