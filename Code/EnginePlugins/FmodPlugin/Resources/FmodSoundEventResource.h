#pragma once

#include <FmodPlugin/FmodPluginDLL.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;
typedef ezTypedResourceHandle<class ezFmodSoundBankResource> ezFmodSoundBankResourceHandle;

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

  /// \brief Creates a new sound event instance of this fmod sound event. May return nullptr, if the event data could not be loaded.
  FMOD::Studio::EventInstance* CreateInstance() const;

  /// \brief Returns the fmod sound event descriptor. May be nullptr, if the sound bank could not be loaded or the event GUID was invalid.
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
    LoadedData() : m_Reader(&m_Storage), m_pEventDescription(nullptr) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    ezFmodSoundBankResourceHandle m_hSoundBank;
    FMOD::Studio::EventDescription* m_pEventDescription;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};


