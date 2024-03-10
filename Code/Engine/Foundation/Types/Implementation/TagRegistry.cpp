#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

static ezTagRegistry s_GlobalRegistry;

ezTagRegistry::ezTagRegistry() = default;

ezTagRegistry& ezTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

const ezTag& ezTagRegistry::RegisterTag(ezStringView sTagString)
{
  ezHashedString TagString;
  TagString.Assign(sTagString);

  return RegisterTag(TagString);
}

const ezTag& ezTagRegistry::RegisterTag(const ezHashedString& sTagString)
{
  EZ_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  const ezTag* pResult = GetTagByName(sTagString);

  if (pResult != nullptr)
    return *pResult;

  const ezUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  ezTag TempTag;
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_sTagString = sTagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(sTagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  ezLog::Debug("Registered Tag '{0}'", sTagString);
  return *m_TagsByIndex.PeekBack();
}

const ezTag* ezTagRegistry::GetTagByName(const ezTempHashedString& sTagString) const
{
  EZ_LOCK(m_TagRegistryMutex);

  auto It = m_RegisteredTags.Find(sTagString);
  if (It.IsValid())
  {
    return &It.Value();
  }

  return nullptr;
}

const ezTag* ezTagRegistry::GetTagByMurmurHash(ezUInt32 uiMurmurHash) const
{
  EZ_LOCK(m_TagRegistryMutex);

  for (ezTag* pTag : m_TagsByIndex)
  {
    if (ezHashingUtils::MurmurHash32String(pTag->GetTagString()) == uiMurmurHash)
    {
      return pTag;
    }
  }

  return nullptr;
}

const ezTag* ezTagRegistry::GetTagByIndex(ezUInt32 uiIndex) const
{
  EZ_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex[uiIndex];
}

ezUInt32 ezTagRegistry::GetNumTags() const
{
  EZ_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex.GetCount();
}

ezResult ezTagRegistry::Load(ezStreamReader& inout_stream)
{
  EZ_LOCK(m_TagRegistryMutex);

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion != 1)
  {
    ezLog::Error("Invalid ezTagRegistry version {0}", uiVersion);
    return EZ_FAILURE;
  }

  ezUInt32 uiNumTags = 0;
  inout_stream >> uiNumTags;

  if (uiNumTags > 16 * 1024)
  {
    ezLog::Error("ezTagRegistry::Load, unreasonable amount of tags {0}, cancelling load.", uiNumTags);
    return EZ_FAILURE;
  }

  ezStringBuilder temp;
  for (ezUInt32 i = 0; i < uiNumTags; ++i)
  {
    inout_stream >> temp;

    RegisterTag(temp);
  }

  return EZ_SUCCESS;
}
