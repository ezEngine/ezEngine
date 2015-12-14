#include <FmodPlugin/PCH.h>
#include <FmodPlugin/FmodSceneModule.h>
#include <Core/World/World.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <FmodPlugin/Components/FmodReverbComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSceneModule, 1, ezRTTIDefaultAllocator<ezFmodSceneModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezFmodSceneModule::InternalStartup()
{
  GetWorld()->CreateComponentManager<ezFmodEventComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezFmodListenerComponentManager>()->SetUserData(this);
  GetWorld()->CreateComponentManager<ezFmodReverbComponentManager>()->SetUserData(this);

  InternalReinit();
}

void ezFmodSceneModule::InternalShutdown()
{
}

void ezFmodSceneModule::InternalUpdate()
{
  ezFmod::GetSingleton()->GetSystem()->update();


}


void ezFmodSceneModule::InternalReinit()
{
}

