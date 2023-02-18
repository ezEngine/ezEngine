#pragma once

#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/FmodPluginDLL.h>

using ezFmodSoundBankResourceHandle = ezTypedResourceHandle<class ezFmodSoundBankResource>;

struct EZ_FMODPLUGIN_DLL ezFmodSoundBankResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_FMODPLUGIN_DLL ezFmodSoundBankResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSoundBankResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezFmodSoundBankResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezFmodSoundBankResource, ezFmodSoundBankResourceDescriptor);

public:
  ezFmodSoundBankResource();
  ~ezFmodSoundBankResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  FMOD::Studio::Bank* m_pSoundBank = nullptr;
  ezDataBuffer* m_pSoundBankData = nullptr;
};

class EZ_FMODPLUGIN_DLL ezFmodSoundBankResourceLoader : public ezResourceTypeLoader
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
    FMOD::Studio::Bank* m_pSoundBank = nullptr;
    ezDataBuffer* m_pSoundbankData = nullptr;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
