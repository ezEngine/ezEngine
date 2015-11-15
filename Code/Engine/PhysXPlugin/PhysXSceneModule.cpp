#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/World/World.h>

#include <PhysXPlugin/Components/StaticMeshComponent.h>
#include <PhysXPlugin/Components/RigidBodyComponent.h>

#include <PxPhysicsAPI.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXSceneModule, 1, ezRTTIDefaultAllocator<ezPhysXSceneModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezPhysXSceneModule::ezPhysXSceneModule()
{


}

void ezPhysXSceneModule::InternalStartup()
{
  GetWorld()->CreateComponentManager<ezStaticMeshComponentManager>();
  GetWorld()->CreateComponentManager<ezRigidBodyComponentManager>();
}

void ezPhysXSceneModule::InternalShutdown()
{
}

void ezPhysXSceneModule::InternalUpdate()
{
}
