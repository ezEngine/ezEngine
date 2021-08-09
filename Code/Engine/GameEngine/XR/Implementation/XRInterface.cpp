#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInterface.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRStageSpace, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRStageSpace::Seated, ezXRStageSpace::Standing)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInterface);
