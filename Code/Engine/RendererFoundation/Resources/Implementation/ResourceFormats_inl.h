
template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::ezGALFormatLookupEntry()
  : m_eStorage(InvalidFormat),
    m_eRenderTarget(InvalidFormat),
    m_eDepthOnlyType(InvalidFormat),
    m_eStencilOnlyType(InvalidFormat),
    m_eDepthStencilType(InvalidFormat),
    m_eVertexAttributeType(InvalidFormat),
    m_eResourceViewType(InvalidFormat)
{

}


template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::ezGALFormatLookupEntry(NativeFormatType Storage)
  : m_eStorage(Storage),
    m_eRenderTarget(InvalidFormat),
    m_eDepthOnlyType(InvalidFormat),
    m_eStencilOnlyType(InvalidFormat),
    m_eDepthStencilType(InvalidFormat),
    m_eVertexAttributeType(InvalidFormat),
    m_eResourceViewType(InvalidFormat)
{

}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(NativeFormatType RenderTargetType)
{
  m_eRenderTarget = RenderTargetType;
  return *this;
}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType DepthOnlyType)
{
  m_eDepthOnlyType = DepthOnlyType;
  return *this;
}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType StencilOnlyType)
{
  m_eStencilOnlyType = StencilOnlyType;
  return *this;
}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(NativeFormatType DepthStencilType)
{
  m_eDepthStencilType = DepthStencilType;
  return *this;
}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(NativeFormatType VertexAttributeType)
{
  m_eVertexAttributeType = VertexAttributeType;
  return *this;
}

template<typename NativeFormatType, NativeFormatType InvalidFormat> ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>& ezGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(NativeFormatType ResourceViewType)
{
  m_eResourceViewType = ResourceViewType;
  return *this;
}


template<typename FormatClass> ezGALFormatLookupTable<FormatClass>::ezGALFormatLookupTable()
{
  for(ezUInt32 i = 0; i < ezGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template<typename FormatClass> const FormatClass& ezGALFormatLookupTable<FormatClass>::GetFormatInfo(ezGALResourceFormat::Enum eFormat) const
{
  return m_Formats[eFormat];
}

template<typename FormatClass> void ezGALFormatLookupTable<FormatClass>::SetFormatInfo(ezGALResourceFormat::Enum eFormat, const FormatClass& NewFormatInfo)
{
  m_Formats[eFormat] = NewFormatInfo;
}