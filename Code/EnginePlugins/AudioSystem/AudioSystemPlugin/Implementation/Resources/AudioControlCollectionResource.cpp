#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>
#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioControlCollectionResource, 1, ezRTTIDefaultAllocator<ezAudioControlCollectionResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAudioControlCollectionResource);

void ezAudioControlCollectionResourceDescriptor::Save(ezStreamWriter& stream) const
{
  constexpr ezUInt8 uiVersion = 1;
  constexpr ezUInt8 uiIdentifier = 0xAC;
  const ezUInt32 uiNumResources = m_Entries.GetCount();

  stream << uiVersion;
  stream << uiIdentifier;
  stream << uiNumResources;

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    ezDefaultMemoryStreamStorage storage(0, ezAudioSystemAllocatorWrapper::GetAllocator());

    {
      ezFileReader file;
      if (file.Open(m_Entries[i].m_sControlFile).Failed())
      {
        ezLog::Error("Could not open audio control file '{0}'", m_Entries[i].m_sControlFile);
        continue;
      }

      ezMemoryStreamWriter writer(&storage);
      ezUInt8 Temp[4 * 1024];
      while (true)
      {
        const ezUInt64 uiRead = file.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));

        if (uiRead == 0)
          break;

        writer.WriteBytes(Temp, uiRead).IgnoreResult();
      }
    }

    stream << m_Entries[i].m_sName;
    stream << m_Entries[i].m_Type;
    stream << m_Entries[i].m_sControlFile;
    stream << storage.GetStorageSize32();
    storage.CopyToStream(stream).IgnoreResult();
  }
}

void ezAudioControlCollectionResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  ezUInt8 uiIdentifier = 0;
  ezUInt32 uiNumResources = 0;

  stream >> uiVersion;
  stream >> uiIdentifier;
  stream >> uiNumResources;

  EZ_ASSERT_DEV(uiIdentifier == 0xAC, "File does not contain a valid ezAudioControlCollectionResourceDescriptor");
  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Entries.SetCount(uiNumResources);

  for (ezUInt32 i = 0; i < uiNumResources; ++i)
  {
    ezUInt32 uiSize = 0;

    stream >> m_Entries[i].m_sName;
    stream >> m_Entries[i].m_Type;
    stream >> m_Entries[i].m_sControlFile;
    stream >> uiSize;

    m_Entries[i].m_pControlBufferStorage = EZ_AUDIOSYSTEM_NEW(ezDefaultMemoryStreamStorage, uiSize, ezAudioSystemAllocatorWrapper::GetAllocator());
    m_Entries[i].m_pControlBufferStorage->ReadAll(stream, uiSize);
  }
}


EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezAudioControlCollectionResource, ezAudioControlCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezAudioControlCollectionResource::ezAudioControlCollectionResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

void ezAudioControlCollectionResource::Register()
{
  if (m_bRegistered)
    return;

  auto* pAudioSystem = ezAudioSystem::GetSingleton();

  for (const auto& entry : m_Collection.m_Entries)
  {
    if (entry.m_sName.IsEmpty() || entry.m_pControlBufferStorage == nullptr)
      continue;

    ezMemoryStreamReader reader(entry.m_pControlBufferStorage);

    switch (entry.m_Type)
    {
      case ezAudioSystemControlType::Trigger:
        pAudioSystem->RegisterTrigger(entry.m_sName, &reader);
        break;
    }
  }
}

void ezAudioControlCollectionResource::Unregister()
{
}

const ezAudioControlCollectionResourceDescriptor& ezAudioControlCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

ezResourceLoadDesc ezAudioControlCollectionResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  {
    Unregister();

    for (auto& entry : m_Collection.m_Entries)
    {
      if (entry.m_pControlBufferStorage != nullptr)
      {
        entry.m_pControlBufferStorage->Clear();
        EZ_AUDIOSYSTEM_DELETE(entry.m_pControlBufferStorage);
      }
    }

    m_Collection.m_Entries.Clear();
    m_Collection.m_Entries.Compact();
  }

  return res;
}

ezResourceLoadDesc ezAudioControlCollectionResource::UpdateContent(ezStreamReader* pStream)
{
  EZ_LOG_BLOCK("ezAudioControlCollectionResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // Skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    *pStream >> sAbsFilePath;
  }

  // Skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  // Load the asset file
  m_Collection.Load(*pStream);

  // Register asset controls in the audio system
  Register();

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezAudioControlCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_Collection.m_Entries.GetHeapMemoryUsage();
}

EZ_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Resources_AudioControlCollectionResource);
