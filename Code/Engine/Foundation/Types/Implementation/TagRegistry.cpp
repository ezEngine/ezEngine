
#include <Foundation/PCH.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Lock.h>

static ezTagRegistry s_GlobalRegistry;

ezTagRegistry::ezTagRegistry()
  : m_uiNextTagIndex(0)
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

  // Build temp tag
  TempTag.m_uiBlockIndex = m_uiNextTagIndex / (sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = m_uiNextTagIndex - (TempTag.m_uiBlockIndex * sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiPreshiftedBit = (static_cast<ezTagSetBlockStorage>(1) << static_cast<ezTagSetBlockStorage>(TempTag.m_uiBitIndex));
  TempTag.m_TagString = TagString;

  m_uiNextTagIndex++;

  // Store the tag
  m_RegisteredTags.Insert(TagString, TempTag);

  if (ResultTag)
  {
    *ResultTag = TempTag;
  }
}

ezResult ezTagRegistry::GetTag(const char* szTagString, ezTag& ResultTag)
{
  ezHashedString TagString;
  TagString.Assign(szTagString);

  return GetTag(TagString, ResultTag);
}

ezResult ezTagRegistry::GetTag(const ezHashedString& TagString, ezTag& ResultTag)
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

