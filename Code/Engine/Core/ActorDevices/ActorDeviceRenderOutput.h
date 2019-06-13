#pragma once

#include <Core/CoreDLL.h>

#include <Core/Actor/ActorDevice.h>

class ezWindowOutputTargetBase;

class EZ_CORE_DLL ezActorDeviceRenderOutput : public ezActorDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorDeviceRenderOutput, ezActorDevice);

public:
  ezActorDeviceRenderOutput();
  ~ezActorDeviceRenderOutput();

  /// \brief Sets whether V-Sync should be enabled
  virtual void SetVSync(bool enable);

  /// \brief Returns whether V-Sync is enabled
  bool GetVSync() const;

  /// \brief Returns the underlying window output target. Can be nullptr.
  virtual ezWindowOutputTargetBase* GetWindowOutputTarget() const = 0;

  virtual void Present() = 0;

protected:
  bool m_bVSync = true;
};
