#include <GameUtils/PCH.h>
#include <GameUtils/Interfaces/PhysicsEngine.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsEngineInterface, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsWorldModuleInterface, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPhysicsEngineInterface* ezPhysicsEngineInterface::s_pInstance = nullptr;

ezPhysicsEngineInterface::ezPhysicsEngineInterface()
{
  EZ_ASSERT_DEV(s_pInstance == nullptr, "Singleton class is already in use");

  s_pInstance = this;
}

ezPhysicsEngineInterface::~ezPhysicsEngineInterface()
{
  s_pInstance = nullptr;
}

