#pragma once

#include <Core/ActorDevices/ActorDeviceRenderOutput.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>

class ezWindowOutputTargetGAL;

class EZ_GAMEENGINE_DLL ezActorDeviceRenderOutputFlatscreen : public ezActorDeviceRenderOutput
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorDeviceRenderOutputFlatscreen, ezActorDeviceRenderOutput);

public:
  ezActorDeviceRenderOutputFlatscreen(ezWindowOutputTargetGAL* pOutputTarget);
  ~ezActorDeviceRenderOutputFlatscreen();

  virtual ezWindowOutputTargetGAL* GetWindowOutputTarget() const override;

  virtual void Present() override;

protected:
  ezWindowOutputTargetGAL* m_pOutputTarget = nullptr;
};
