#pragma once

#include <Core/ResourceManager/Resource.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

class ezRandom;

using ezMiniAudioSoundResourceHandle = ezTypedResourceHandle<class ezMiniAudioSoundResource>;

struct EZ_MINIAUDIOPLUGIN_DLL ezMiniAudioSoundResourceDescriptor{
  // empty, these types of resources must be loaded from file
};

class EZ_MINIAUDIOPLUGIN_DLL ezMiniAudioSoundResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSoundResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezMiniAudioSoundResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezMiniAudioSoundResource, ezMiniAudioSoundResourceDescriptor);

public:
  ezMiniAudioSoundResource();
  ~ezMiniAudioSoundResource();

  const ezDataBuffer& GetAudioData() const;
  const ezDataBuffer& GetAudioData(ezRandom& ref_rng) const;

  bool GetLoop() const { return m_bLoop; }
  float GetVolume(ezRandom& ref_rng) const;
  float GetPitch(ezRandom& ref_rng) const;
  bool GetSpatialize() const { return m_bSpatialize; }
  float GetDopplerFactor() const { return m_fDopplerFactor; }
  float GetMinDistance() const { return m_fMinDistance; }
  float GetMaxDistance() const { return m_fMaxDistance; }
  float GetRolloff() const { return m_fRolloff; }

  /// \brief Instantiates the sound, all arguments are optional.
  ezMiniAudioSoundInstance* InstantiateSound(ezRandom* pRng, ezWorld* pWorld, ezComponentHandle hComponent);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezHybridArray<ezDataBuffer, 1> m_AudioData;

  ezString m_sSoundGroup;
  bool m_bLoop = false;
  float m_fMinVolume = 1.0f;
  float m_fMaxVolume = 1.0f;
  float m_fMinDistance = 1.0f;
  float m_fMaxDistance = 10.0f;
  float m_fMinPitch = 1.0f;
  float m_fMaxPitch = 1.0f;
  bool m_bSpatialize = true;
  float m_fDopplerFactor = 1.0f;
  float m_fRolloff = 1.0f;
};
