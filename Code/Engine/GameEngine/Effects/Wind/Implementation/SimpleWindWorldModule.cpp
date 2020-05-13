#include <GameEnginePCH.h>

#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezSimpleWindWorldModule);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleWindWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimpleWindWorldModule::ezSimpleWindWorldModule(ezWorld* pWorld)
  : ezWindWorldModuleInterface(pWorld)
{
  m_vFallbackWind.SetZero();
}

ezSimpleWindWorldModule::~ezSimpleWindWorldModule() = default;

ezVec3 ezSimpleWindWorldModule::GetWindAt(const ezVec3& vPosition)
{
  return m_vFallbackWind;
}

void ezSimpleWindWorldModule::SetFallbackWind(const ezVec3& vWind)
{
  m_vFallbackWind = vWind;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindWorldModule);
