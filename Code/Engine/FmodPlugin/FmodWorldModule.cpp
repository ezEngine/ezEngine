#include <PCH.h>
#include <FmodPlugin/FmodWorldModule.h>
#include <Core/World/World.h>
#include <FmodPlugin/Components/FmodEventComponent.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <FmodPlugin/Components/FmodReverbComponent.h>
#include <FmodPlugin/Resources/FmodSoundBankResource.h>

EZ_IMPLEMENT_WORLD_MODULE(ezFmodSceneModule);

ezFmodSoundBankResourceHandle hRes[5];

ezFmodSceneModule::ezFmodSceneModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{

}

void ezFmodSceneModule::Initialize()
{
  //hRes[0] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("Sound/Soundbanks/Desktop/FleetOps.bank");
  //hRes[1] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Master Bank.strings.bank");
  //hRes[2] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Surround_Ambience.bank");
  //hRes[3] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/UI_Menu.bank");
  //hRes[4] = ezResourceManager::LoadResource<ezFmodSoundBankResource>("SoundBanks/Weapons.bank");

  for (int i = 0; i < 5; ++i)
  {
    if (hRes[i].IsValid())
    {
      ezResourceLock<ezFmodSoundBankResource> pRes(hRes[i], ezResourceAcquireMode::NoFallback);
    }
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezFmodSceneModule::UpdateSound, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f; // don't really care about priority

    RegisterUpdateFunction(desc);
  }
}


void ezFmodSceneModule::Deinitialize()
{
  for (int i = 0; i < 5; ++i)
  {
    hRes[i].Invalidate();
  }
}

void ezFmodSceneModule::UpdateSound(const ezWorldModule::UpdateContext& context)
{
  ezFmod::GetSingleton()->GetSystem()->update();
}




EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_FmodWorldModule);

