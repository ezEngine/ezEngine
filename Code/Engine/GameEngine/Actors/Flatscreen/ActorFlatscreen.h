#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Actor/Actor.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Actors/Flatscreen/ActorDeviceRenderOutputFlatscreen.h>

class ezWindow;

class EZ_GAMEENGINE_DLL ezActorFlatscreen : public ezActor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorFlatscreen, ezActor);

public:
  ezWindow* GetWindow();

private: // functions called directly by ezActorManagerFlatscreen
  friend class ezActorManagerFlatscreen;

  ezActorFlatscreen(const char* szActorName, const char* szGroupName, ezUniquePtr<ezWindow>&& pWindow);

protected:
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual void Update() override;

  ezUniquePtr<ezWindow> m_pWindow;
  ezUniquePtr<ezWindowOutputTargetGAL> m_OutputTarget;


};
