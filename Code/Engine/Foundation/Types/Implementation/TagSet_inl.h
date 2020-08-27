#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>


// Template specialization to be able to use ezTagSet properties as EZ_SET_MEMBER_PROPERTY.
template <typename T>
struct ezContainerSubTypeResolver<ezTagSetTemplate<T>>
{
  typedef const char* Type;
};


template <typename Class>
class ezMemberSetProperty<Class, ezTagSet, const char*> : public ezTypedSetProperty<typename ezTypeTraits<const char*>::NonConstReferenceType>
{
public:
  typedef ezTagSet Container;
  typedef ezConstCharPtr Type;
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef const Container& (*GetConstContainerFunc)(const Class* pInstance);
  typedef Container& (*GetContainerFunc)(Class* pInstance);

  ezMemberSetProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : ezTypedSetProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an set property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      ezAbstractSetProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) override
  {
    EZ_ASSERT_DEBUG(
      m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, void* pObject) override
  {
    EZ_ASSERT_DEBUG(
      m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, void* pObject) override
  {
    EZ_ASSERT_DEBUG(
      m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveByName(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, void* pObject) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, ezHybridArray<ezVariant, 16>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : m_ConstGetter(static_cast<const Class*>(pInstance)))
    {
      out_keys.PushBack(ezVariant(value));
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};


template <typename BlockStorageAllocator>
ezTagSetTemplate<BlockStorageAllocator>::Iterator::Iterator(const ezTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd)
  : m_pTagSet(pSet)
  , m_uiIndex(0)
{
  if (!bEnd)
  {
    m_uiIndex = m_pTagSet->GetTagBlockStart() * (sizeof(ezTagSetBlockStorage) * 8);

    if (m_pTagSet->IsEmpty())
      m_uiIndex = 0xFFFFFFFF;
    else
    {
      if (!IsBitSet())
        operator++();
    }
  }
  else
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::Iterator::IsBitSet() const
{
  ezTag TempTag;
  TempTag.m_uiBlockIndex = m_uiIndex / (sizeof(ezTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = m_uiIndex - (TempTag.m_uiBlockIndex * sizeof(ezTagSetBlockStorage) * 8);

  return m_pTagSet->IsSet(TempTag);
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Iterator::operator++()
{
  const ezUInt32 uiMax = m_pTagSet->GetTagBlockEnd() * (sizeof(ezTagSetBlockStorage) * 8);

  do
  {
    ++m_uiIndex;
  } while (m_uiIndex < uiMax && !IsBitSet());

  if (m_uiIndex >= uiMax)
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
const ezTag* ezTagSetTemplate<BlockStorageAllocator>::Iterator::operator*() const
{
  return ezTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
const ezTag* ezTagSetTemplate<BlockStorageAllocator>::Iterator::operator->() const
{
  return ezTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
ezTagSetTemplate<BlockStorageAllocator>::ezTagSetTemplate()
{
  SetTagBlockStart(ezSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::operator==(const ezTagSetTemplate& other) const
{
  return m_TagBlocks == other.m_TagBlocks && m_TagBlocks.GetUserData<ezUInt32>() == other.m_TagBlocks.GetUserData<ezUInt32>();
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::operator!=(const ezTagSetTemplate& other) const
{
  return !(*this == other);
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Set(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (!IsTagInAllocatedRange(Tag))
  {
    const ezUInt32 uiTagBlockStart = GetTagBlockStart();
    const ezUInt32 uiNewBlockStart = (uiTagBlockStart != ezSmallInvalidIndex) ? ezMath::Min(Tag.m_uiBlockIndex, uiTagBlockStart) : Tag.m_uiBlockIndex;
    const ezUInt32 uiNewBlockIndex = (uiTagBlockStart != ezSmallInvalidIndex) ? ezMath::Max(Tag.m_uiBlockIndex, uiTagBlockStart) : Tag.m_uiBlockIndex;

    Reallocate(uiNewBlockStart, uiNewBlockIndex);
  }

  ezUInt64& tagBlock = m_TagBlocks[Tag.m_uiBlockIndex - GetTagBlockStart()];

  const ezUInt64 bitMask = EZ_BIT(Tag.m_uiBitIndex);  
  const bool bBitWasSet = ((tagBlock & bitMask) != 0);

  tagBlock |= bitMask;

  if (!bBitWasSet)
  {
    IncreaseTagCount();
  }
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Remove(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(Tag))
  {
    ezUInt64& tagBlock = m_TagBlocks[Tag.m_uiBlockIndex - GetTagBlockStart()];

    const ezUInt64 bitMask = EZ_BIT(Tag.m_uiBitIndex);
    const bool bBitWasSet = ((tagBlock & bitMask) != 0);

    tagBlock &= ~bitMask;

    if (bBitWasSet)
    {
      DecreaseTagCount();
    }
  }
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsSet(const ezTag& Tag) const
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be checked!");

  if (IsTagInAllocatedRange(Tag))
  {
    return (m_TagBlocks[Tag.m_uiBlockIndex - GetTagBlockStart()] & EZ_BIT(Tag.m_uiBitIndex)) != 0;
  }
  else
  {
    return false;
  }
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsAnySet(const ezTagSetTemplate& OtherSet) const
{
  // If any of the sets is empty nothing can match
  if (IsEmpty() || OtherSet.IsEmpty())
    return false;

  // Calculate range to compare
  const ezUInt32 uiMaxBlockStart = ezMath::Max(GetTagBlockStart(), OtherSet.GetTagBlockStart());
  const ezUInt32 uiMinBlockEnd = ezMath::Min(GetTagBlockEnd(), OtherSet.GetTagBlockEnd());

  if (uiMaxBlockStart > uiMinBlockEnd)
    return false;

  for (ezUInt32 i = uiMaxBlockStart; i < uiMinBlockEnd; ++i)
  {
    const ezUInt32 uiThisBlockStorageIndex = i - GetTagBlockStart();
    const ezUInt32 uiOtherBlockStorageIndex = i - OtherSet.GetTagBlockStart();

    if ((m_TagBlocks[uiThisBlockStorageIndex] & OtherSet.m_TagBlocks[uiOtherBlockStorageIndex]) != 0)
    {
      return true;
    }
  }

  return false;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezUInt32 ezTagSetTemplate<BlockStorageAllocator>::GetNumTagsSet() const
{
  return GetTagCount();
}

template <typename BlockStorageAllocator>
EZ_ALWAYS_INLINE bool ezTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  return GetTagCount() == 0;
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Clear()
{
  m_TagBlocks.Clear();
  SetTagBlockStart(ezSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::SetByName(const char* szTag)
{
  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTag);
  Set(tag);
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::RemoveByName(const char* szTag)
{
  if (const ezTag* tag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTag)))
  {
    Remove(*tag);
  }
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsSetByName(const char* szTag) const
{
  if (const ezTag* tag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTag)))
  {
    return IsSet(*tag);
  }

  return false;
}

template <typename BlockStorageAllocator>
EZ_ALWAYS_INLINE bool ezTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const ezTag& Tag) const
{
  return Tag.m_uiBlockIndex >= GetTagBlockStart() && Tag.m_uiBlockIndex < GetTagBlockEnd();
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Reallocate(ezUInt32 uiNewTagBlockStart, ezUInt32 uiNewMaxBlockIndex)
{
  EZ_ASSERT_DEV(uiNewTagBlockStart < ezSmallInvalidIndex, "Tag block start is too big");
  const ezUInt16 uiNewBlockArraySize = static_cast<ezUInt16>((uiNewMaxBlockIndex - uiNewTagBlockStart) + 1);

  // Early out for non-filled tag sets
  if (IsEmpty())
  {
    m_TagBlocks.SetCount(uiNewBlockArraySize);
    SetTagBlockStart(static_cast<ezUInt16>(uiNewTagBlockStart));

    return;
  }

  EZ_ASSERT_DEBUG(uiNewTagBlockStart <= GetTagBlockStart(), "New block start must be smaller or equal to current block start!");

  ezSmallArray<ezUInt64, 32, BlockStorageAllocator> helperArray;
  helperArray.SetCount(uiNewBlockArraySize);

  const ezUInt32 uiOldBlockStartOffset = GetTagBlockStart() - uiNewTagBlockStart;

  // Copy old data to the new array
  ezMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  // Use array ptr copy assignment so it doesn't modify the user data in m_TagBlocks
  m_TagBlocks = helperArray.GetArrayPtr();
  SetTagBlockStart(static_cast<ezUInt16>(uiNewTagBlockStart));
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezUInt16 ezTagSetTemplate<BlockStorageAllocator>::GetTagBlockStart() const
{
  return m_TagBlocks.GetUserData<UserData>().m_uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezUInt16 ezTagSetTemplate<BlockStorageAllocator>::GetTagBlockEnd() const
{
  return static_cast<ezUInt16>(GetTagBlockStart() + m_TagBlocks.GetCount());
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezTagSetTemplate<BlockStorageAllocator>::SetTagBlockStart(ezUInt16 uiTagBlockStart)
{
  m_TagBlocks.GetUserData<UserData>().m_uiTagBlockStart = uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE ezUInt16 ezTagSetTemplate<BlockStorageAllocator>::GetTagCount() const
      {
  return m_TagBlocks.GetUserData<UserData>().m_uiTagCount;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezTagSetTemplate<BlockStorageAllocator>::SetTagCount(ezUInt16 uiTagCount)
{
  m_TagBlocks.GetUserData<UserData>().m_uiTagCount = uiTagCount;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezTagSetTemplate<BlockStorageAllocator>::IncreaseTagCount()
{
  m_TagBlocks.GetUserData<UserData>().m_uiTagCount++;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
EZ_ALWAYS_INLINE void ezTagSetTemplate<BlockStorageAllocator>::DecreaseTagCount()
{
  m_TagBlocks.GetUserData<UserData>().m_uiTagCount--;
}

static ezTypeVersion s_TagSetVersion = 1;

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
void ezTagSetTemplate<BlockStorageAllocator>::Save(ezStreamWriter& stream) const
{
  const ezUInt16 uiNumTags = static_cast<ezUInt16>(GetNumTagsSet());
  stream << uiNumTags;

  stream.WriteVersion(s_TagSetVersion);

  for (Iterator it = GetIterator(); it.IsValid(); ++it)
  {
    const ezTag* pTag = *it;

    stream << pTag->m_TagString;
  }
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
void ezTagSetTemplate<BlockStorageAllocator>::Load(ezStreamReader& stream, ezTagRegistry& registry)
{
  ezUInt16 uiNumTags = 0;
  stream >> uiNumTags;

  // Manually read version value since 0 can be a valid version here
  ezTypeVersion version;
  stream.ReadWordValue(&version);

  if (version == 0)
  {
    for (ezUInt32 i = 0; i < uiNumTags; ++i)
    {
      ezUInt32 uiTagMurmurHash = 0;
      stream >> uiTagMurmurHash;

      if (const ezTag* pTag = registry.GetTagByMurmurHash(uiTagMurmurHash))
      {
        Set(*pTag);
      }
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < uiNumTags; ++i)
    {
      ezHashedString tagString;
      stream >> tagString;

      const ezTag& tag = registry.RegisterTag(tagString);
      Set(tag);
    }
  }
}
