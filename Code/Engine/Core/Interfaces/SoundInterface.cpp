#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Foundation/Configuration/Singleton.h>

ezResult ezSoundInterface::PlaySound(ezStringView sResourceID, const ezTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    return pSoundInterface->OneShotSound(sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return EZ_FAILURE;
}


