#include <GameEnginePCH.h>

#include <GameEngine/Interfaces/WindWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWindWorldModuleInterface, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWindWorldModuleInterface::ezWindWorldModuleInterface(ezWorld* pWorld)
    : ezWorldModule(pWorld)
{
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Interfaces_WindWorldModule);

