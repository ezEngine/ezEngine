#pragma once

#include <Core/ActorDevices/ActorDeviceRenderOutput.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>

class ezWindowOutputTargetGAL;

class EZ_GAMEENGINE_DLL ezActorDeviceRenderOutputGAL : public ezActorDeviceRenderOutput
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorDeviceRenderOutputGAL, ezActorDeviceRenderOutput);

public:
  ezActorDeviceRenderOutputGAL(ezWindowOutputTargetGAL* pOutputTarget);
  ~ezActorDeviceRenderOutputGAL();

  virtual ezWindowOutputTargetGAL* GetWindowOutputTarget() const override;

  virtual void Present() override;

protected:
  ezWindowOutputTargetGAL* m_pOutputTarget = nullptr;
};
