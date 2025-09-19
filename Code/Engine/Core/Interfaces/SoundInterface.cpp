#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>

ezResult ezSoundInterface::PlaySound(ezWorld* pWorld, ezStringView sResourceID, const ezTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>())
  {
    return pSoundInterface->OneShotSound(pWorld, sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return EZ_FAILURE;
}

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_Sound, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(PlaySound, In, "World", In, "Resource", In, "GlobalPosition", In, "GlobalRotation", In, "Pitch", In, "Volume", In, "BlockToLoad")->AddAttributes(
      new ezFunctionArgumentAttributes(3, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(4, new ezDefaultValueAttribute(1.0f)),
      new ezFunctionArgumentAttributes(5, new ezDefaultValueAttribute(true))
    ),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("Sound"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezScriptExtensionClass_Sound::PlaySound(ezWorld* pWorld, ezStringView sResourceID, const ezVec3& vGlobalPos, const ezQuat& qGlobalRot, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  ezSoundInterface::PlaySound(pWorld, sResourceID, ezTransform(vGlobalPos, qGlobalRot), fPitch, fVolume, bBlockIfNotLoaded).IgnoreResult();
}


EZ_STATICLINK_FILE(Core, Core_Interfaces_SoundInterface);
