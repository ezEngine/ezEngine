
EZ_ALWAYS_INLINE ID3D11Device* ezGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

EZ_ALWAYS_INLINE ID3D11Device3* ezGALDeviceDX11::GetDXDevice3() const
{
  return m_pDevice3;
}

EZ_ALWAYS_INLINE ID3D11DeviceContext* ezGALDeviceDX11::GetDXImmediateContext() const
{
  return m_pImmediateContext;
}

EZ_ALWAYS_INLINE IDXGIFactory1* ezGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

EZ_ALWAYS_INLINE const ezGALFormatLookupTableDX11& ezGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D11Query* ezGALDeviceDX11::GetTimestamp(ezGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<ezUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}
