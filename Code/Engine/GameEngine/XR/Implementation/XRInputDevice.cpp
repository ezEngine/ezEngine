#include <GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRInputDevice.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezXRInputDevice, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInputDevice);
