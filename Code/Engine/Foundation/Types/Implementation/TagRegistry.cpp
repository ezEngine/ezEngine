
#include <Foundation/PCH.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Logging/Log.h>

static ezTagRegistry s_GlobalRegistry;

ezTagRegistry::ezTagRegistry()
{
}

ezTagRegistry& ezTagRegistry::GetGlobalRegistry()
{
  return s_GlobalRegistry;
}

void ezTagRegistry::RegisterTag(const char* szTagString, ezTag* ResultTag /*= nullptr*/)
{
  ezHashedString TagString;
  TagString.Assign(szTagString);

  RegisterTag(TagString, ResultTag);
}

void ezTagRegistry::RegisterTag(const ezHashedString& TagString, ezTag* ResultTag /*= nullptr*/)
{
  EZ_LOCK(m_TagRegistryMutex);

  // Early out if the tag is already registered
  ezTag TempTag;
  if (GetTag(TagString, TempTag).Succeeded())
  {
    if (ResultTag)
    {
      *ResultTag = TempTag;
    }

    return;
  }

  const ezUInt32 uiNextTagIndex = m_TagsByIndex.GetCount();

  // Build temp tag
  TempTag.m_uiBlockIndex = uiNextTagIndex / (sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiPreshiftedBit = (static_cast<ezTagSetBlockStorage>(1) << static_cast<ezTagSetBlockStorage>(TempTag.m_uiBitIndex));
  TempTag.m_TagString = TagString;

  // Store the tag
  auto it = m_RegisteredTags.Insert(TagString, TempTag);

  m_TagsByIndex.PushBack(&it.Value());

  if (ResultTag)
  {
    *ResultTag = TempTag;
  }

  ezLog::Debug("Registered Tag '%s'", TagString.GetData());
}

ezResult ezTagRegistry::GetTag(const char* szTagString, ezTag& ResultTag) const
{
  ezHashedString TagString;
  TagString.Assign(szTagString);

  return GetTag(TagString, ResultTag);
}

ezResult ezTagRegistry::GetTag(const ezHashedString& TagString, ezTag& ResultTag) const
{
  EZ_LOCK(m_TagRegistryMutex);


  auto It = m_RegisteredTags.Find(TagString);
  if (It.IsValid())
  {
    ResultTag = It.Value();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

const ezTag* ezTagRegistry::GetTagByIndex(ezUInt32 uiIndex) const
{
  EZ_LOCK(m_TagRegistryMutex);
  return m_TagsByIndex[uiIndex];
}

EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_TagRegistry);