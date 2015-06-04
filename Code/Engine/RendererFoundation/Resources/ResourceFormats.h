
#pragma once

#include <RendererFoundation/Basics.h>

struct ezGALResourceFormat
{
  enum Enum
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

    ENUM_COUNT
  };


  // General format Meta-Informations:

  /// \brief The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  EZ_RENDERERFOUNDATION_DLL static ezUInt32 GetBitsPerElement(ezGALResourceFormat::Enum format)          { return BitsPerElement[format]; }

  /// \brief The number of color channels this format contains.
  EZ_RENDERERFOUNDATION_DLL static ezUInt8 GetChannelCount(ezGALResourceFormat::Enum format)  { return ChannelCount[format]; }

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

private:
  
  static const ezUInt8 BitsPerElement[ezGALResourceFormat::ENUM_COUNT];

  static const ezUInt8 ChannelCount[ezGALResourceFormat::ENUM_COUNT];
};


template<typename NativeFormatType, NativeFormatType InvalidFormat> class ezGALFormatLookupEntry
{
public:

  EZ_FORCE_INLINE ezGALFormatLookupEntry();

  EZ_FORCE_INLINE ezGALFormatLookupEntry(NativeFormatType Storage);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType RenderTargetType);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType DepthOnlyType);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType StencilOnlyType);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType DepthStencilType);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType VertexAttributeType);

  EZ_FORCE_INLINE ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType ResourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;

};

// Reusable table class to store lookup information (from ezGALResourceFormat to the various formats for texture/buffer storage, views)
template<typename FormatClass> class ezGALFormatLookupTable
{
public:

  ezGALFormatLookupTable();

  EZ_FORCE_INLINE const FormatClass& GetFormatInfo(ezGALResourceFormat::Enum eFormat) const;

  EZ_FORCE_INLINE void SetFormatInfo(ezGALResourceFormat::Enum eFormat, const FormatClass& NewFormatInfo);

private:

  FormatClass m_Formats[ezGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>