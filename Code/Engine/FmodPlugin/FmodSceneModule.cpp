#include <FmodPlugin/PCH.h>
#include <FmodPlugin/FmodSceneModule.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSceneModule, 1, ezRTTIDefaultAllocator<ezFmodSceneModule>);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezFmodSceneModule::InternalStartup()
{
  //GetWorld()->CreateComponentManager<ezPxStaticActorComponentManager>()->SetUserData(this);

  InternalReinit();
}

void ezFmodSceneModule::InternalShutdown()
{
}

void ezFmodSceneModule::InternalUpdate()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  ezFmod::GetSingleton()->GetSystem()->update();
}


void ezFmodSceneModule::InternalReinit()
{
  FMOD::Studio::EventDescription* cancelDescription = nullptr;
  FMOD_CHECK(ezFmod::GetSingleton()->GetSystem()->getEvent("event:/UI/Cancel", &cancelDescription));

  FMOD::Studio::EventInstance* cancelInstance = nullptr;
  FMOD_CHECK(cancelDescription->createInstance(&cancelInstance));

  cancelInstance->start();
}

