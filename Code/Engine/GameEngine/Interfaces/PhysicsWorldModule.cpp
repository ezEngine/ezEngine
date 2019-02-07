#include <GameEnginePCH.h>

#include <GameEngine/Interfaces/PhysicsWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysicsWorldModuleInterface, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsAddImpulse);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsAddImpulse, 1, ezRTTIDefaultAllocator<ezMsgPhysicsAddImpulse>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgPhysicsAddForce);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgPhysicsAddForce, 1, ezRTTIDefaultAllocator<ezMsgPhysicsAddForce>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBuildStaticMesh);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBuildStaticMesh, 1, ezRTTIDefaultAllocator<ezMsgBuildStaticMesh>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


EZ_STATICLINK_FILE(GameEngine, GameEngine_Interfaces_PhysicsWorldModule);

