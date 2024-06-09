#pragma once

#include <RendererFoundation/Resources/Query.h>

class ezGALQueryVulkan : public ezGALQuery
{
public:
  EZ_ALWAYS_INLINE ezUInt32 GetID() const;
  EZ_ALWAYS_INLINE vk::QueryPool GetPool() const { return nullptr; } // TODO

protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALQueryVulkan(const ezGALQueryCreationDescription& Description);
  ~ezGALQueryVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezUInt32 m_uiID;
};

#include <RendererVulkan/Resources/Implementation/QueryVulkan_inl.h>
