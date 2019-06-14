#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Actor/ActorManager.h>

class ezActorFlatscreen;
struct ezWindowCreationDesc;

class EZ_GAMEENGINE_DLL ezActorManagerFlatscreen : public ezActorManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezActorManagerFlatscreen, ezActorManager);

public:
  ezActorManagerFlatscreen();
  ~ezActorManagerFlatscreen();

  ezActorFlatscreen* CreateFlatscreenActor(const char* szActorName, const void* pCreatedBy, const ezWindowCreationDesc& windowDesc);
};
