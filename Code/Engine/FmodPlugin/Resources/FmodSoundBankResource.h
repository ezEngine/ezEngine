#pragma once

#include <FmodPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

typedef ezResourceHandle<class ezFmodSoundBankResource> ezFmodSoundBankResourceHandle;

struct EZ_FMODPLUGIN_DLL ezFmodSoundBankResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_FMODPLUGIN_DLL ezFmodSoundBankResource : public ezResource<ezFmodSoundBankResource, ezFmodSoundBankResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSoundBankResource, ezResourceBase);

public:
  ezFmodSoundBankResource();
  ~ezFmodSoundBankResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezFmodSoundBankResourceDescriptor& descriptor) override;

private:
  FMOD::Studio::Bank* m_pSoundBank;
};

class EZ_FMODPLUGIN_DLL ezFmodSoundBankResourceLoader : public ezResourceTypeLoader
{
public:

  struct LoadedData
  {
    LoadedData() : m_Reader(&m_Storage), m_pSoundBank(nullptr) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
    FMOD::Studio::Bank* m_pSoundBank;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};


