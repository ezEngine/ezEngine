
#pragma once

#include <RendererFoundation/Resources/Fence.h>

struct ID3D11Fence;
struct ID3D11DeviceContext4;

class EZ_RENDERERDX11_DLL ezGALFenceDX11 : public ezGALFence
{
public:
  EZ_ALWAYS_INLINE ID3D11Fence* GetDXFence() const { return m_pDXFence; }

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALFenceDX11();

  virtual ~ezGALFenceDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezUInt64 initialValue) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual ezUInt64 GetCompletedValuePlatform() const override;

  virtual void WaitPlatform(ezUInt64 value) const override;

  virtual void SignalPlatform(ezUInt64 value) const override;

  ID3D11Fence* m_pDXFence;
  ID3D11DeviceContext4* m_pDXDeviceContext;
  HANDLE m_FenceEvent = nullptr;
};
