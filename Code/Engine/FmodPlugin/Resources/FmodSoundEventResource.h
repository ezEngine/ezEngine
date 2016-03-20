#pragma once

#include <FmodPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;
typedef ezTypedResourceHandle<class ezFmodSoundBankResource> ezFmodSoundBankResourceHandle;

struct EZ_FMODPLUGIN_DLL ezFmodSoundEventResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_FMODPLUGIN_DLL ezFmodSoundEventResource : public ezResource<ezFmodSoundEventResource, ezFmodSoundEventResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSoundEventResource, ezResourceBase);

public:
  ezFmodSoundEventResource();
  ~ezFmodSoundEventResource();

  FMOD::Studio::EventInstance* CreateInstance() const;

  FMOD::Studio::EventDescription* GetDescriptor() const { return m_pEventDescription; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezFmodSoundEventResourceDescriptor& descriptor) override;

private:
  ezFmodSoundBankResourceHandle m_hSoundBank;
  FMOD::Studio::EventDescription* m_pEventDescription;
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

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};


