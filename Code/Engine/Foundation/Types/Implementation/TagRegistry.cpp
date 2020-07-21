#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>

static ezTagRegistry s_GlobalRegistry;

ezTagRegistry::ezTagRegistry() {}

ezTagRegistry& ezTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

const ezTag& ezTagRegistry::RegisterTag(const char* szTagString)
{
  ezHashedString TagString;
  TagString.Assign(szTagString);

  return RegisterTag(TagString);
}

const ezTag& ezTagRegistry::RegisterTag(const ezHashedString& TagString)
{
  EZ_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  const ezTag* pResult = GetTagByName(TagString);

  if (pResult != nullptr)
    return *pResult;

  const ezUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  ezTag TempTag;
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiPreshiftedBit = (static_cast<ezTagSetBlockStorage>(1) << static_cast<ezTagSetBlockStorage>(TempTag.m_uiBitIndex));
  TempTag.m_TagString = TagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(TagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  ezLog::Debug("Registered Tag '{0}'", TagString);
  return *m_TagsByIndex.PeekBack();
}

const ezTag* ezTagRegistry::GetTagByName(const ezTempHashedString& TagString) const
{
  EZ_LOCK(m_TagRegistryMutex);

  auto It = m_RegisteredTags.Find(TagString);
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
    if (ezHashingUtils::MurmurHash32String(pTag->GetTagString().GetData()) == uiMurmurHash)
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

ezResult ezTagRegistry::Load(ezStreamReader& stream)
{
  EZ_LOCK(m_TagRegistryMutex);

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  if (uiVersion != 1)
  {
    ezLog::Error("Invalid ezTagRegistry version {0}", uiVersion);
    return EZ_FAILURE;
  }

  ezUInt32 uiNumTags = 0;
  stream >> uiNumTags;

  if (uiNumTags > 16 * 1024)
  {
    ezLog::Error("ezTagRegistry::Load, unreasonable amount of tags {0}, cancelling load.", uiNumTags);
    return EZ_FAILURE;
  }

  ezStringBuilder temp;
  for (ezUInt32 i = 0; i < uiNumTags; ++i)
  {
    stream >> temp;

    RegisterTag(temp);
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_TagRegistry);

