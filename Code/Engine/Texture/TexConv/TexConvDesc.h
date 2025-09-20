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

/// \brief Complete texture conversion configuration with all processing options.
///
/// This structure contains all settings needed to convert source images into optimized
/// textures for runtime use. It handles input specification, format conversion, quality
/// settings, mipmap generation, and platform-specific optimizations.
///
/// **Basic Usage Pattern:**
/// ```cpp
/// ezTexConvDesc desc;
/// desc.m_InputFiles.PushBack("diffuse.png");
/// desc.m_OutputType = ezTexConvOutputType::Texture2D;
/// desc.m_Usage = ezTexConvUsage::Color;
/// desc.m_CompressionMode = ezTexConvCompressionMode::HighQuality;
/// desc.m_TargetPlatform = ezTexConvTargetPlatform::PC;
/// // Process with ezTexConvProcessor...
/// ```
class EZ_TEXTURE_DLL ezTexConvDesc
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexConvDesc);

public:
  ezTexConvDesc() = default;

  // Input specification
  ezHybridArray<ezString, 4> m_InputFiles;                          ///< Source image file paths to process
  ezDynamicArray<ezImage> m_InputImages;                            ///< Pre-loaded source images (alternative to file paths)

  ezHybridArray<ezTexConvSliceChannelMapping, 6> m_ChannelMappings; ///< Channel routing for multi-input processing

  // Output configuration
  ezEnum<ezTexConvOutputType> m_OutputType;         ///< Type of texture to generate (2D, Cubemap, 3D, etc.)
  ezEnum<ezTexConvTargetPlatform> m_TargetPlatform; ///< Target platform for format optimization

  // Multi-resolution output
  ezUInt32 m_uiLowResMipmaps = 0;             ///< Number of low-resolution mipmap levels to generate separately
  ezUInt32 m_uiThumbnailOutputResolution = 0; ///< Size for thumbnail generation (0 = no thumbnail)

  // Format and compression
  ezEnum<ezTexConvUsage> m_Usage;                     ///< Intended usage (Color, Normal, Linear, etc.) affects format selection
  ezEnum<ezTexConvCompressionMode> m_CompressionMode; ///< Quality vs file size trade-off

  // Resolution control
  ezUInt32 m_uiMinResolution = 16;       ///< Minimum texture dimension (prevents over-downscaling)
  ezUInt32 m_uiMaxResolution = 1024 * 8; ///< Maximum texture dimension (prevents excessive memory usage)
  ezUInt32 m_uiDownscaleSteps = 0;       ///< Number of 2x downscaling steps to apply

  // Mipmap generation
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;    ///< Mipmap generation strategy
  ezEnum<ezTextureFilterSetting> m_FilterMode; ///< Runtime filtering quality (ez formats only)
  ezEnum<ezImageAddressMode> m_AddressModeU;   ///< Horizontal texture wrapping mode
  ezEnum<ezImageAddressMode> m_AddressModeV;   ///< Vertical texture wrapping mode
  ezEnum<ezImageAddressMode> m_AddressModeW;   ///< Depth texture wrapping mode (3D textures)
  bool m_bPreserveMipmapCoverage = false;      ///< Maintain alpha coverage for alpha testing
  float m_fMipmapAlphaThreshold = 0.5f;        ///< Alpha threshold for coverage preservation

  // Image processing options
  ezUInt8 m_uiDilateColor = 0;      ///< Color dilation steps (fills transparent areas)
  bool m_bFlipHorizontal = false;   ///< Mirror image horizontally
  bool m_bPremultiplyAlpha = false; ///< Pre-multiply RGB by alpha for correct blending
  float m_fHdrExposureBias = 0.0f;  ///< HDR exposure adjustment (stops)
  float m_fMaxValue = 64000.f;      ///< HDR value clamping

  // Runtime metadata
  ezUInt64 m_uiAssetHash = 0;    ///< Content hash for cache invalidation
  ezUInt16 m_uiAssetVersion = 0; ///< Asset version for dependency tracking

  // Advanced features
  ezString m_sTextureAtlasDescFile;               ///< Path to texture atlas description file
  ezEnum<ezTexConvBumpMapFilter> m_BumpMapFilter; ///< Bump map specific filtering
};
