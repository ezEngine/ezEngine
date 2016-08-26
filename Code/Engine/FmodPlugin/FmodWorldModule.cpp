#include <FmodPlugin/PCH.h>
#include <FmodPlugin/FmodWorldModule.h>
#include <Core/World/World.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <FmodPlugin/Components/FmodReverbComponent.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFmodSceneModule, 1, ezRTTIDefaultAllocator<ezFmodSceneModule>)
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

ezFmodSoundBankResourceHandle hRes[5];

void ezFmodSceneModule::InternalStartup()
{
  //hRes[0] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("Sound/Soundbanks/Desktop/FleetOps.bank");
  //hRes[1] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Master Bank.strings.bank");
  //hRes[2] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Surround_Ambience.bank");
  //hRes[3] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/UI_Menu.bank");
  //hRes[4] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Weapons.bank");

  GetWorld()->GetOrCreateComponentManager<ezFmodEventComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezFmodListenerComponentManager>()->SetUserData(this);
  GetWorld()->GetOrCreateComponentManager<ezFmodReverbComponentManager>()->SetUserData(this);

  for (int i = 0; i < 5; ++i)
  {
    if (hRes[i].IsValid())
    {
      ezResourceLock<ezFmodSoundBankResource> pRes(hRes[i], ezResourceAcquireMode::NoFallback);
    }
  }

  InternalReinit();
}

void ezFmodSceneModule::InternalAfterWorldDestruction()
{
  for (int i = 0; i < 5; ++i)
  {
    hRes[i].Invalidate();
  }
}

void ezFmodSceneModule::InternalUpdate()
{
  ezFmod::GetSingleton()->GetSystem()->update();


}


void ezFmodSceneModule::InternalReinit()
{
}

