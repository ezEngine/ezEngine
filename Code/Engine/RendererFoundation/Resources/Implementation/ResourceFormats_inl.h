
// static
EZ_ALWAYS_INLINE ezUInt32 ezGALResourceFormat::GetBitsPerElement(ezGALResourceFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
EZ_ALWAYS_INLINE ezUInt8 ezGALResourceFormat::GetChannelCount(ezGALResourceFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
EZ_FORCE_INLINE bool ezGALResourceFormat::IsDepthFormat(ezGALResourceFormat::Enum format)
{
  return format == DFloat || format == D16 || format == D24S8;
}

// static
EZ_FORCE_INLINE bool ezGALResourceFormat::IsStencilFormat(Enum format)
{
  return format == D24S8;
}

// static
EZ_FORCE_INLINE bool ezGALResourceFormat::IsSrgb(ezGALResourceFormat::Enum format)
{
  return format == BGRAUByteNormalizedsRGB || format == RGBAUByteNormalizedsRGB || format == BC1sRGB || format == BC2sRGB || format == BC3sRGB ||
         format == BC7UNormalizedsRGB;
}

EZ_FORCE_INLINE bool ezGALResourceFormat::IsIntegerFormat(Enum format)
{
  switch (format)
  {
    // D16 is actually a 16 bit unorm format
    case ezGALResourceFormat::D16:
    // 32bit, 4 channel
    case ezGALResourceFormat::RGBAUInt:
    case ezGALResourceFormat::RGBAInt:
    // 32bit, 3 channel
    case ezGALResourceFormat::RGBUInt:
    case ezGALResourceFormat::RGBInt:
    // 16bit, 4 channel
    case ezGALResourceFormat::RGBAUShort:
    case ezGALResourceFormat::RGBAShort:
    // 16bit, 2 channel
    case ezGALResourceFormat::RGUInt:
    case ezGALResourceFormat::RGInt:
    // packed 32bit, 4 channel
    case ezGALResourceFormat::RGB10A2UInt:
    // 8bit, 4 channel
    case ezGALResourceFormat::RGBAUByte:
    case ezGALResourceFormat::RGBAByte:
    // 16bit, 2 channel
    case ezGALResourceFormat::RGUShort:
    case ezGALResourceFormat::RGShort:
    // 8bit, 2 channel
    case ezGALResourceFormat::RGUByte:
    case ezGALResourceFormat::RGByte:
    // 32bit, 1 channel
    case ezGALResourceFormat::RUInt:
    case ezGALResourceFormat::RInt:
    // 16bit, 1 channel
    case ezGALResourceFormat::RUShort:
    case ezGALResourceFormat::RShort:
    // 8bit, 1 channel
    case ezGALResourceFormat::RUByte:
    case ezGALResourceFormat::RByte:
      return true;
    default:
      return false;
  }
}

EZ_FORCE_INLINE bool ezGALResourceFormat::IsSignedFormat(Enum format)
{
  switch (format)
  {
    case RGBAFloat:
    case RGBAInt:
    case RGBFloat:
    case RGBInt:
    case RGBAHalf:
    case RGBAShort:
    case RGBAShortNormalized:
    case RGFloat:
    case RGInt:
    case RGB10A2UIntNormalized:
    case RG11B10Float:
    case RGBAByteNormalized:
    case RGBAByte:
    case RGHalf:
    case RGShort:
    case RGShortNormalized:
    case RGByte:
    case RGByteNormalized:
    case RFloat:
    case RInt:
    case RHalf:
    case RShort:
    case RShortNormalized:
    case RByte:
    case RByteNormalized:
    case BC4Normalized:
    case BC5Normalized:
    case BC6Float:
      return true;
    default:
      return false;
  }
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::ezGALFormatLookupEntry()
  : m_eStorage(InvalidFormat)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::ezGALFormatLookupEntry(NativeFormatType storage)
  : m_eStorage(storage)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(
  NativeFormatType renderTargetType)
{
  m_eRenderTarget = renderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType depthOnlyType)
{
  m_eDepthOnlyType = depthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType stencilOnlyType)
{
  m_eStencilOnlyType = stencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(
  NativeFormatType depthStencilType)
{
  m_eDepthStencilType = depthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(
  NativeFormatType vertexAttributeType)
{
  m_eVertexAttributeType = vertexAttributeType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(
  NativeFormatType resourceViewType)
{
  m_eResourceViewType = resourceViewType;
  return *this;
}


template <typename FormatClass>
ezGALFormatLookupTable<FormatClass>::ezGALFormatLookupTable()
{
  for (ezUInt32 i = 0; i < ezGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template <typename FormatClass>
const FormatClass& ezGALFormatLookupTable<FormatClass>::GetFormatInfo(ezGALResourceFormat::Enum format) const
{
  return m_Formats[format];
}

template <typename FormatClass>
void ezGALFormatLookupTable<FormatClass>::SetFormatInfo(ezGALResourceFormat::Enum format, const FormatClass& newFormatInfo)
{
  m_Formats[format] = newFormatInfo;
}
