
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

struct EZ_RENDERERFOUNDATION_DLL ezGALResourceFormat
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Invalid = 0,

    RGBAFloat,
    XYZWFloat = RGBAFloat,
    RGBAUInt,
    RGBAInt,

    RGBFloat,
    XYZFloat = RGBFloat,
    UVWFloat = RGBFloat,
    RGBUInt,
    RGBInt,

    B5G6R5UNormalized,
    BGRAUByteNormalized,
    BGRAUByteNormalizedsRGB,

    RGBAHalf,
    XYZWHalf = RGBAHalf,
    RGBAUShort,
    RGBAUShortNormalized,
    RGBAShort,
    RGBAShortNormalized,

    RGFloat,
    XYFloat = RGFloat,
    UVFloat = RGFloat,
    RGUInt,
    RGInt,

    RGB10A2UInt,
    RGB10A2UIntNormalized,
    RG11B10Float,

    RGBAUByteNormalized,
    RGBAUByteNormalizedsRGB,
    RGBAUByte,
    RGBAByteNormalized,
    RGBAByte,

    RGHalf,
    XYHalf = RGHalf,
    UVHalf = RGHalf,
    RGUShort,
    RGUShortNormalized,
    RGShort,
    RGShortNormalized,
    RGUByte,
    RGUByteNormalized,
    RGByte,
    RGByteNormalized,

    DFloat,

    RFloat,
    RUInt,
    RInt,
    RHalf,
    RUShort,
    RUShortNormalized,
    RShort,
    RShortNormalized,
    RUByte,
    RUByteNormalized,
    RByte,
    RByteNormalized,

    AUByteNormalized,

    D16,
    D24S8,

    BC1,
    BC1sRGB,
    BC2,
    BC2sRGB,
    BC3,
    BC3sRGB,
    BC4UNormalized,
    BC4Normalized,
    BC5UNormalized,
    BC5Normalized,
    BC6UFloat,
    BC6Float,
    BC7UNormalized,
    BC7UNormalizedsRGB,

    // careful, if anything gets added here, make sure to update IsBlockCompressed()

    ENUM_COUNT,

    Default = Invalid
  };


  // General format Meta-Informations:

  /// \brief The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  static ezUInt32 GetBitsPerElement(ezGALResourceFormat::Enum format);

  /// \brief The number of color channels this format contains.
  static ezUInt8 GetChannelCount(ezGALResourceFormat::Enum format);

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

  /// \brief Returns whether the given resource format is a depth format
  static bool IsDepthFormat(ezGALResourceFormat::Enum format);
  static bool IsStencilFormat(ezGALResourceFormat::Enum format);

  static bool IsSrgb(ezGALResourceFormat::Enum format);

  static bool IsBlockCompressed(ezGALResourceFormat::Enum format);

  /// \brief Returns whether the given resource format returns integer values when sampled (e.g. RUShort). Note that normalized formats like RGUShortNormalized are not considered integer formats as they return float values in the [0..1] range when sampled.
  static bool IsIntegerFormat(ezGALResourceFormat::Enum format);

  /// \brief Returns whether the given resource format can store negative values.
  static bool IsSignedFormat(ezGALResourceFormat::Enum format);

  static bool IsFloatFormat(ezGALResourceFormat::Enum format);

private:
  static const ezUInt8 s_BitsPerElement[ezGALResourceFormat::ENUM_COUNT];

  static const ezUInt8 s_ChannelCount[ezGALResourceFormat::ENUM_COUNT];
};

template <typename NativeFormatType, NativeFormatType InvalidFormat>
class ezGALFormatLookupEntry
{
public:
  inline ezGALFormatLookupEntry();

  inline ezGALFormatLookupEntry(NativeFormatType storage);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType vertexAttributeType);

  inline ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;
};

// Reusable table class to store lookup information (from ezGALResourceFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class ezGALFormatLookupTable
{
public:
  ezGALFormatLookupTable();

  EZ_ALWAYS_INLINE const FormatClass& GetFormatInfo(ezGALResourceFormat::Enum format) const;

  EZ_ALWAYS_INLINE void SetFormatInfo(ezGALResourceFormat::Enum format, const FormatClass& newFormatInfo);

private:
  FormatClass m_Formats[ezGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>
