#pragma once

#include <Foundation/Types/Types.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TextureDLL.h>

/// \brief Categorizes image formats by their storage and compression method.
///
/// This classification helps determine how image data is laid out in memory
/// and which processing algorithms are applicable.
struct EZ_TEXTURE_DLL ezImageFormatType
{
  enum Enum
  {
    UNKNOWN,          ///< Format type could not be determined
    LINEAR,           ///< Pixels stored linearly, uncompressed (e.g., RGBA8)
    BLOCK_COMPRESSED, ///< Pixels stored in compressed blocks (e.g., BC1, ASTC)
    PLANAR            ///< Channels stored in separate planes (e.g., YUV formats)
  };
};

/// \brief Specifies the data type and interpretation of channel values.
///
/// This determines how raw channel bits are interpreted as numeric values
/// and affects precision, range, and rendering behavior.
struct EZ_TEXTURE_DLL ezImageFormatDataType
{
  enum Enum
  {
    FLOAT,         ///< IEEE floating point values
    UINT,          ///< Unsigned integer values (0 to max)
    SINT,          ///< Signed integer values (-max to +max)
    UNORM,         ///< Unsigned normalized values (0.0 to 1.0)
    SNORM,         ///< Signed normalized values (-1.0 to 1.0)
    DEPTH_STENCIL, ///< Depth and/or stencil buffer values
    GENERIC,       ///< Format-specific interpretation
    NONE           ///< No data type applicable
  };
};

/// \brief Identifies individual channels within an image format.
struct EZ_TEXTURE_DLL ezImageFormatChannel
{
  enum Enum
  {
    R = 0, ///< Red channel
    G,     ///< Green channel
    B,     ///< Blue channel
    A,     ///< Alpha channel
    D,     ///< Depth channel
    S,     ///< Stencil channel
    COUNT
  };
};

/// \brief Comprehensive enumeration of all supported pixel formats with utility functions.
///
/// This struct provides both format enumeration and extensive utility functions for working with
/// different pixel formats. It handles format conversion queries, memory layout calculations,
/// and format property inspection.
///
/// **Format Categories:**
/// - **Linear formats**: Channels stored interleaved per pixel (e.g., RGBARGBARGBA...)
/// - **Block compressed**: Pixels grouped into compressed blocks (BC1-7, ASTC formats)
/// - **Planar formats**: Channels stored in separate memory planes (YUV video formats)
///
/// **Common Usage Patterns:**
/// ```cpp
/// // Query format properties
/// bool isCompressed = ezImageFormat::IsCompressed(ezImageFormat::BC1_UNORM);
/// ezUInt32 bitsPerPixel = ezImageFormat::GetBitsPerPixel(format);
///
/// // Calculate memory requirements
/// ezUInt64 rowPitch = ezImageFormat::GetRowPitch(format, width);
/// ezUInt64 slicePitch = ezImageFormat::GetDepthPitch(format, width, height);
///
/// // Format conversion queries
/// bool canConvert = ezImageFormat::IsCompatible(sourceFormat, targetFormat);
/// ezImageFormat::Enum srgbVersion = ezImageFormat::AsSrgb(linearFormat);
/// ```
///
/// **Block Compressed Formats:**
/// Block compressed formats store pixels in fixed-size blocks (4x4 for BC formats, variable for ASTC).
/// This provides significant memory savings but requires special handling during processing.
///
/// **Planar Formats:**
/// Planar formats like NV12 store different channels in separate memory planes. For example,
/// NV12 has a luma (Y) plane and an interleaved chroma (UV) plane. Use GetPlaneSubFormat()
/// to get the format description for individual planes.
struct EZ_TEXTURE_DLL ezImageFormat
{
  enum Enum : ezUInt16
  {
    UNKNOWN,

    // 32b per component, 4 components
    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,

    // 32b per component, 3 components
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,

    // 16b per component, 4 components
    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,

    // 16b per component, 3 components
    R16G16B16_UNORM,

    // 32b per component, 2 components
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,

    // Pseudo depth-stencil formats
    D32_FLOAT_S8X24_UINT,

    // 10b and 11b per component
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R11G11B10_FLOAT,

    // 8b per component, 4 components
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,

    B8G8R8A8_UNORM,
    B8G8R8A8_UNORM_SRGB,
    B8G8R8X8_UNORM,
    B8G8R8X8_UNORM_SRGB,

    // 16b per component, 2 components
    R16G16_FLOAT,
    R16G16_UNORM,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,

    // 32b per component, 1 component
    D32_FLOAT,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,

    // Mixed 24b/8b formats
    D24_UNORM_S8_UINT,

    // 8b per component, three components
    R8G8B8_UNORM,
    R8G8B8_UNORM_SRGB,
    B8G8R8_UNORM,
    B8G8R8_UNORM_SRGB,

    // 8b per component, two components
    R8G8_UNORM,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,

    // 16b per component, one component

    R16_FLOAT,
    D16_UNORM,
    R16_UNORM,
    R16_UINT,
    R16_SNORM,
    R16_SINT,

    // 8b per component, one component
    R8_UNORM,
    R8_UINT,
    R8_SNORM,
    R8_SINT,

    // Block compression formats
    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UF16,
    BC6H_SF16,
    BC7_UNORM,
    BC7_UNORM_SRGB,

    // ASTC formats
    ASTC_4x4_UNORM,
    ASTC_4x4_UNORM_SRGB,
    ASTC_5x4_UNORM,
    ASTC_5x4_UNORM_SRGB,
    ASTC_5x5_UNORM,
    ASTC_5x5_UNORM_SRGB,
    ASTC_6x5_UNORM,
    ASTC_6x5_UNORM_SRGB,
    ASTC_6x6_UNORM,
    ASTC_6x6_UNORM_SRGB,
    ASTC_8x5_UNORM,
    ASTC_8x5_UNORM_SRGB,
    ASTC_8x6_UNORM,
    ASTC_8x6_UNORM_SRGB,
    ASTC_10x5_UNORM,
    ASTC_10x5_UNORM_SRGB,
    ASTC_10x6_UNORM,
    ASTC_10x6_UNORM_SRGB,
    ASTC_8x8_UNORM,
    ASTC_8x8_UNORM_SRGB,
    ASTC_10x8_UNORM,
    ASTC_10x8_UNORM_SRGB,
    ASTC_10x10_UNORM,
    ASTC_10x10_UNORM_SRGB,
    ASTC_12x10_UNORM,
    ASTC_12x10_UNORM_SRGB,
    ASTC_12x12_UNORM,
    ASTC_12x12_UNORM_SRGB,

    // 16bpp formats
    B4G4R4A4_UNORM,
    B4G4R4A4_UNORM_SRGB,
    A4B4G4R4_UNORM,
    A4B4G4R4_UNORM_SRGB,
    B5G6R5_UNORM,
    B5G6R5_UNORM_SRGB,
    B5G5R5A1_UNORM,
    B5G5R5A1_UNORM_SRGB,
    B5G5R5X1_UNORM,
    B5G5R5X1_UNORM_SRGB,
    A1B5G5R5_UNORM,
    A1B5G5R5_UNORM_SRGB,
    X1B5G5R5_UNORM,
    X1B5G5R5_UNORM_SRGB,

    // Planar formats
    NV12,
    P010,

    NUM_FORMATS,

    Default = UNKNOWN
  };

  using StorageType = ezUInt16;

  /// \brief Returns the name of the given format.
  ///
  /// The returned string is guaranteed to be stable across engine versions and thus suitable for serialization.
  static const char* GetName(Enum format);

  /// \brief Returns number of planes in the format, or 1 for non-planar formats.
  static ezUInt32 GetPlaneCount(Enum format);

  /// \brief Returns the number of bits per pixel of the given format. If the format's bpp is non-integral, the returned value rounded up to
  /// to the next integer.
  static ezUInt32 GetBitsPerPixel(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Exact pixel size in bits. May be non-integral for some compressed formats.
  static float GetExactBitsPerPixel(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Returns the block size in bits. For uncompressed formats, a block is considered a single pixel.
  static ezUInt32 GetBitsPerBlock(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Number of channels (r, g, b, a, depth, stencil) supported by this format.
  static ezUInt32 GetNumChannels(Enum format);

  /// \brief Bitmask of each channel of the format. This is not defined for some formats, and may return 0.
  static ezUInt32 GetChannelMask(Enum format, ezImageFormatChannel::Enum c);

  /// \brief Returns the number of bits for each channel of the format.
  static ezUInt32 GetBitsPerChannel(Enum format, ezImageFormatChannel::Enum c);

  /// \brief If applicable, returns a bitmask for the red component of the format.
  static ezUInt32 GetRedMask(Enum format);

  /// \brief If applicable, returns a bitmask for the green component of the format.
  static ezUInt32 GetGreenMask(Enum format);

  /// \brief If applicable, returns a bitmask for the blue component of the format.
  static ezUInt32 GetBlueMask(Enum format);

  /// \brief If applicable, returns a bitmask for the alpha component of the format.
  static ezUInt32 GetAlphaMask(Enum format);

  /// \brief Block width of a compressed format. Defaults to 1 for uncompressed formats.
  static ezUInt32 GetBlockWidth(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Block height of a compressed format. Defaults to 1 for uncompressed formats.
  static ezUInt32 GetBlockHeight(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Block depth of a compressed format. Defaults to 1 for uncompressed formats.
  static ezUInt32 GetBlockDepth(Enum format, ezUInt32 uiPlaneIndex = 0);

  /// \brief Returns the data type represented by a format.
  static ezImageFormatDataType::Enum GetDataType(Enum format);

  /// \brief Returns true if the format is compressed.
  static bool IsCompressed(Enum format);

  /// \brief Returns true if the format is a depth format.
  static bool IsDepth(Enum format);

  /// \brief Returns whether the given format is an sRGB format.
  static bool IsSrgb(Enum format);

  /// \brief Returns true if the format is a stencil format.
  static bool IsStencil(Enum format);

  /// \brief Returns the corresponding sRGB format if one exists; otherwise returns the unmodified format.
  static Enum AsSrgb(Enum format);

  /// \brief Returns the corresponding linear format if one exists; otherwise returns the unmodified format.
  static Enum AsLinear(Enum format);

  /// \brief Computes the number of blocks in X direction (compressed) or pixels (if uncompressed) for a given width (in pixels).
  static ezUInt32 GetNumBlocksX(Enum format, ezUInt32 uiWidth, ezUInt32 uiPlaneIndex = 0);

  /// \brief Computes the number of blocks in Y direction (compressed) or pixels (if uncompressed) for a given height (in pixels).
  static ezUInt32 GetNumBlocksY(Enum format, ezUInt32 uiHeight, ezUInt32 uiPlaneIndex = 0);

  /// \brief Computes the number of blocks in Z direction (compressed) or pixels (if uncompressed) for a given height (in pixels).
  static ezUInt32 GetNumBlocksZ(Enum format, ezUInt32 uiDepth, ezUInt32 uiPlaneIndex = 0);

  /// \brief Computes the size in bytes of a row of blocks (compressed) or pixels (if uncompressed) of the given width.
  static ezUInt64 GetRowPitch(Enum format, ezUInt32 uiWidth, ezUInt32 uiPlaneIndex = 0);

  /// \brief Computes the size in bytes of a 2D slice of blocks (compressed) or pixels (if uncompressed) of the given width and height.
  static ezUInt64 GetDepthPitch(Enum format, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiPlaneIndex = 0);

  /// \brief Returns the type of the image format.
  static ezImageFormatType::Enum GetType(Enum format);

  /// \brief Finds a format matching the given component masks.
  static ezImageFormat::Enum FromPixelMask(
    ezUInt32 uiRedMask, ezUInt32 uiGreenMask, ezUInt32 uiBlueMask, ezUInt32 uiAlphaMask, ezUInt32 uiBitsPerPixel);

  /// \brief Returns the format of a subplane of a given format.
  static ezImageFormat::Enum GetPlaneSubFormat(ezImageFormat::Enum format, ezUInt32 uiPlaneIndex);

  /// \brief Returns true if the data formats are compatible, i.e. can be copied into one another
  static bool IsCompatible(Enum left, Enum right);

  /// \brief Returns true if the most high-res miplevel requires block alignment
  static bool RequiresFirstLevelBlockAlignment(Enum format);
};

EZ_DEFINE_AS_POD_TYPE(ezImageFormat::Enum);
