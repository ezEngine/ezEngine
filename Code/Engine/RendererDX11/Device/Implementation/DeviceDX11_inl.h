
ID3D11Device* ezGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

IDXGIFactory1* ezGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

const ezGALFormatLookupTableDX11& ezGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}