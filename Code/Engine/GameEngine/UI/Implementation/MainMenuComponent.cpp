#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/UI/MainMenuComponent.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezMainMenuComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input/UI"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

// static
ezComponentHandle ezMainMenuComponent::FindInWorld(ezWorld& ref_world)
{
  EZ_LOCK(ref_world.GetReadMarker());

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    ezMainMenuComponent* pMenu = nullptr;
    if (it->TryGetComponentOfBaseType(pMenu))
    {
      return pMenu->GetHandle();
    }
  }

  return ezComponentHandle();
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_UI_Implementation_MainMenuComponent);
