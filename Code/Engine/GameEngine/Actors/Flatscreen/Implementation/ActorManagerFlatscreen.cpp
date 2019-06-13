#include <GameEnginePCH.h>

#include <GameEngine/Actors/Flatscreen/ActorFlatscreen.h>
#include <GameEngine/Actors/Flatscreen/ActorManagerFlatscreen.h>
#include <GameEngine/GameApplication/GameApplicationBase.h>
#include <GameEngine/GameState/GameStateWindow.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorManagerFlatscreen, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorManagerFlatscreen::ezActorManagerFlatscreen() = default;
ezActorManagerFlatscreen::~ezActorManagerFlatscreen() = default;


ezActorFlatscreen* ezActorManagerFlatscreen::CreateFlatscreenActor(
  const char* szActorName, const char* szGroupName, const ezWindowCreationDesc& windowDesc0)
{
  ezWindowCreationDesc windowDesc = windowDesc0;
  //ezGameApplicationBase::GetGameApplicationBaseInstance()->AdjustWindowCreation(windowDesc);

  ezUniquePtr<ezGameStateWindow> pWindow = EZ_DEFAULT_NEW(ezGameStateWindow, windowDesc, [] {});
  pWindow->ResetOnClickClose([]() { ezGameApplicationBase::GetGameApplicationBaseInstance()->RequestQuit(); });

  ezUniquePtr<ezActorFlatscreen> pActorFlatscreen = EZ_DEFAULT_NEW(ezActorFlatscreen, szActorName, szGroupName, std::move(pWindow));
  ezActorFlatscreen* pActorToReturn = pActorFlatscreen.Borrow();

  AddActor(std::move(pActorFlatscreen));

  return pActorToReturn;
}

void ezActorManagerFlatscreen::DestroyFlatscreenActor(ezActorFlatscreen* pActor)
{
  DestroyActor(pActor);
}
