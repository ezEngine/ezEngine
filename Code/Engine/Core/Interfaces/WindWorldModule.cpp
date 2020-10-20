#include <CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWindWorldModuleInterface, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWindWorldModuleInterface::ezWindWorldModuleInterface(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

EZ_STATICLINK_FILE(Core, Core_Interfaces_WindWorldModule);
