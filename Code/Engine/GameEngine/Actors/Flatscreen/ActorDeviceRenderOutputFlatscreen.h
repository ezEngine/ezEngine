#pragma once

#include <Core/ActorDevices/ActorDeviceRenderOutput.h>
#include <GameApplication/WindowOutputTarget.h>

class ezActorDeviceRenderOutputFlatscreen : public ezActorDeviceRenderOutput
{
EZ_ADD_DYNAMIC_REFLECTION(ezActorDeviceRenderOutputFlatscreen, ezActorDeviceRenderOutput);

public:
  ezActorDeviceRenderOutputFlatscreen(ezWindowOutputTargetGAL* pOutputTarget);
  ~ezActorDeviceRenderOutputFlatscreen();

  virtual ezWindowOutputTargetGAL* GetWindowOutputTarget() const override;

protected:
  ezWindowOutputTargetGAL* m_pOutputTarget = nullptr;
};
