#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezWorld;
class ezParticleSystemInstance;

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectManager
{
  EZ_DECLARE_SINGLETON(ezParticleEffectManager);
public:
  ezParticleEffectManager();

  ezParticleSystemInstance* CreateParticleSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed);
  void DestroyParticleSystemInstance(ezParticleSystemInstance* pInstance);

  void Shutdown();

private:
  ezDeque<ezParticleSystemInstance*> m_ParticleSystemFreeList;
};