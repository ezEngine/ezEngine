#pragma once

#include <RendererFoundation/Resources/ReadbackBuffer.h>

struct ID3D11Buffer;

class ezGALReadbackBufferDX11 : public ezGALReadbackBuffer
{
public:
  EZ_ALWAYS_INLINE ID3D11Buffer* GetDXBuffer() const { return m_pDXBuffer; }

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALReadbackBufferDX11(const ezGALBufferCreationDescription& Description);
  ~ezGALReadbackBufferDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  ID3D11Buffer* m_pDXBuffer = nullptr;
  DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_UNKNOWN; // Only applicable for index buffers
};
