#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/Assets/AssetFileHeader.h>


class EZ_CORE_DLL ezCustomData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomData, ezReflectedClass);

public:
  virtual void Load(ezStreamReader& inout_stream);
  virtual void Save(ezStreamWriter& inout_stream) const;
};


// \todo A macro creating a derived type from ezCustomDataResource

template<typename T>
class ezCustomDataResource : public ezResource
{
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCustomDataResource<T>, T);

public:
  ezCustomDataResource();
  ~ezCustomDataResource();

  T& GetData() { return m_Data; }
  const T& GetData() const { return m_Data; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:

  T m_Data;
};


// \todo Move everything below to a separate _inl.h file

template<typename T>
ezCustomDataResource<T>::ezCustomDataResource() : ezResource(DoUpdate::OnAnyThread, 1)
{
}

template<typename T>
ezCustomDataResource<T>::~ezCustomDataResource() = default;

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  return res;
}

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    T dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

template<typename T>
void ezCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

template<typename T>
EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezCustomDataResource<T>, T)
{
  m_Data = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

