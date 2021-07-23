
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALFence : public ezRefCounted
{
public:
  ezUInt64 GetCompletedValue() const;

  void Wait(ezUInt64 value) const;

  void Signal(ezUInt64 value) const;

protected:
  friend class ezGALDevice;

  ezGALFence();

  virtual ~ezGALFence();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezUInt64 initialValue) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezUInt64 GetCompletedValuePlatform() const = 0;

  virtual void WaitPlatform(ezUInt64 value) const = 0;

  virtual void SignalPlatform(ezUInt64 value) const = 0;
};
