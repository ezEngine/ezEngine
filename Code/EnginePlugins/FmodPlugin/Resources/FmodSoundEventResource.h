#pragma once

#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/FmodPluginDLL.h>

using ezFmodSoundEventResourceHandle = ezTypedResourceHandle<class ezFmodSoundEventResource>;
using ezFmodSoundBankResourceHandle = ezTypedResourceHandle<class ezFmodSoundBankResource>;

struct EZ_FMODPLUGIN_DLL ezFmodSoundEventResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_FMODPLUGIN_DLL ezFmodSoundEventResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSoundEventResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezFmodSoundEventResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezFmodSoundEventResource, ezFmodSoundEventResourceDescriptor);

public:
  ezFmodSoundEventResource();
  ~ezFmodSoundEventResource();

  /// \brief Creates an instance of this sound event and plays it.
  ///
  /// This is only allowed for events that are not looped, otherwise EZ_FAILURE is returned.
  ezResult PlayOnce(const ezTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f) const;

  /// \brief Creates a new sound event instance of this FMOD sound event. May return nullptr, if the event data could not be loaded.
  FMOD::Studio::EventInstance* CreateInstance() const;

  /// \brief Returns the FMOD sound event descriptor. May be nullptr, if the sound bank could not be loaded or the event GUID was invalid.
  FMOD::Studio::EventDescription* GetDescriptor() const { return m_pEventDescription; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezFmodSoundBankResourceHandle m_hSoundBank;
  FMOD::Studio::EventDescription* m_pEventDescription = nullptr;
};

class EZ_FMODPLUGIN_DLL ezFmodSoundEventResourceLoader : public ezResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)

    {
    }

    ezDefaultMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezFmodSoundBankResourceHandle m_hSoundBank;
    FMOD::Studio::EventDescription* m_pEventDescription = nullptr;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
