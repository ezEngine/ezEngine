#include <PCH.h>

#include <GameEngine/Interfaces/WindWorldModule.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezWindWorldModuleInterface);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWindWorldModuleInterface, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWindWorldModuleInterface::ezWindWorldModuleInterface(ezWorld* pWorld)
    : ezWorldModule(pWorld)
{
  m_vFallbackWind.SetZero();
}

 ezWindWorldModuleInterface::~ezWindWorldModuleInterface() = default;

ezVec3 ezWindWorldModuleInterface::GetWindAt(const ezVec3& vPosition)
{
  return m_vFallbackWind;
}

void ezWindWorldModuleInterface::SetFallbackWind(const ezVec3& vWind)
{
  m_vFallbackWind = vWind;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Interfaces_WindWorldModule);

