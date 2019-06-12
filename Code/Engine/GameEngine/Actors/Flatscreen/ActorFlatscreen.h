#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Actor/Actor.h>

class ezWindow;

class EZ_GAMEENGINE_DLL ezActorFlatscreen : public ezActor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorFlatscreen, ezActor);

private: // functions called directly by ezActorManagerFlatscreen
  friend class ezActorManagerFlatscreen;

  ezActorFlatscreen(const char* szActorName, ezUniquePtr<ezWindow>&& pWindow);

protected:
  virtual void Activate() override;
  virtual void Deactivate() override;

  ezUniquePtr<ezWindow> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetGAL> m_OutputTarget;
  ezUniquePtr<ezActorDeviceRenderOutputFlatscreen> m_pRenderOutput;
};
