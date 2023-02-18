
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
