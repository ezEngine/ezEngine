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
class ezMemberSetProperty<Class, ezTagSet, const char*>
    : public ezTypedSetProperty<typename ezTypeTraits<const char*>::NonConstReferenceType>
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
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
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
    m_uiIndex = pSet->m_uiTagBlockStart * (sizeof(ezTagSetBlockStorage) * 8);

    if (pSet->IsEmpty())
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
  TempTag.m_uiPreshiftedBit = (static_cast<ezTagSetBlockStorage>(1) << static_cast<ezTagSetBlockStorage>(TempTag.m_uiBitIndex));

  return m_pTagSet->IsSet(TempTag);
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Iterator::operator++()
{
  const ezUInt32 uiMax = (m_pTagSet->m_uiTagBlockStart + m_pTagSet->m_TagBlocks.GetCount()) * (sizeof(ezTagSetBlockStorage) * 8);

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
    : m_uiTagBlockStart(0xFFFFFFFFu)
{
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Set(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (!IsTagInAllocatedRange(Tag))
  {
    const ezUInt32 uiNewBlockStart =
        (m_uiTagBlockStart != 0xFFFFFFFFu) ? ezMath::Min(Tag.m_uiBlockIndex, m_uiTagBlockStart) : Tag.m_uiBlockIndex;
    const ezUInt32 uiNewBlockIndex =
        (m_uiTagBlockStart != 0xFFFFFFFFu) ? ezMath::Max(Tag.m_uiBlockIndex, m_uiTagBlockStart) : Tag.m_uiBlockIndex;

    Reallocate(uiNewBlockStart, uiNewBlockIndex);
  }

  m_TagBlocks[Tag.m_uiBlockIndex - m_uiTagBlockStart] |= Tag.m_uiPreshiftedBit;
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Remove(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(Tag))
  {
    m_TagBlocks[Tag.m_uiBlockIndex - m_uiTagBlockStart] &= (~Tag.m_uiPreshiftedBit);
  }
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsSet(const ezTag& Tag) const
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be checked!");

  if (IsTagInAllocatedRange(Tag))
  {
    return (m_TagBlocks[Tag.m_uiBlockIndex - m_uiTagBlockStart] & Tag.m_uiPreshiftedBit) != 0;
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
  if (m_uiTagBlockStart == 0xFFFFFFFFu || OtherSet.m_uiTagBlockStart == 0xFFFFFFFFu)
    return false;

  // Calculate range to compare
  const ezUInt32 uiMaxBlockStart = ezMath::Max(m_uiTagBlockStart, OtherSet.m_uiTagBlockStart);
  const ezUInt32 uiMinBlockEnd =
      ezMath::Min(m_uiTagBlockStart + m_TagBlocks.GetCount(), OtherSet.m_uiTagBlockStart + OtherSet.m_TagBlocks.GetCount());

  if (uiMaxBlockStart > uiMinBlockEnd)
    return false;

  for (ezUInt32 i = uiMaxBlockStart; i < uiMinBlockEnd; ++i)
  {
    const ezUInt32 uiThisBlockStorageIndex = i - m_uiTagBlockStart;
    const ezUInt32 uiOtherBlockStorageIndex = i - OtherSet.m_uiTagBlockStart;

    if ((m_TagBlocks[uiThisBlockStorageIndex] & OtherSet.m_TagBlocks[uiOtherBlockStorageIndex]) != 0)
    {
      return true;
    }
  }

  return false;
}


template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
ezUInt32 ezTagSetTemplate<BlockStorageAllocator>::GetNumTagsSet() const
{
  // early out, if it is completely cleared
  if (m_uiTagBlockStart == 0xFFFFFFFFu)
    return 0;

  ezUInt32 count = 0;

  for (ezUInt32 i = 0; i < m_TagBlocks.GetCount(); ++i)
  {
    const ezUInt64 value = m_TagBlocks[i];

    for (ezUInt32 bit = 0; bit < 64; ++bit)
    {
      const ezUInt64 pattern = value >> bit;

      if ((pattern & 1U) != 0) // lowest bit is set ?
      {
        ++count;
      }
    }
  }

  if (count == 0)
  {
    // make sure we don't need to do the full check next time again
    m_uiTagBlockStart = 0xFFFFFFFFu;
  }

  return count;
}

template <typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  // early out, if it is completely cleared
  if (m_uiTagBlockStart == 0xFFFFFFFFu)
    return true;

  for (ezUInt32 i = 0; i < m_TagBlocks.GetCount(); ++i)
  {
    const ezUInt64 value = m_TagBlocks[i];

    for (ezUInt32 bit = 0; bit < 64; ++bit)
    {
      const ezUInt64 pattern = value >> bit;

      if ((pattern & 1U) != 0) // lowest bit is set ?
      {
        return false;
      }
    }
  }

  // make sure we don't need to do the full check next time again
  m_uiTagBlockStart = 0xFFFFFFFFu;
  return true;
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Clear()
{
  m_TagBlocks.Clear();
  m_uiTagBlockStart = 0xFFFFFFFFu;
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
bool ezTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const ezTag& Tag) const
{
  return Tag.m_uiBlockIndex >= m_uiTagBlockStart && Tag.m_uiBlockIndex < (m_uiTagBlockStart + m_TagBlocks.GetCount());
}

template <typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Reallocate(ezUInt32 uiNewTagBlockStart, ezUInt32 uiNewMaxBlockIndex)
{
  const ezUInt32 uiNewBlockArraySize = (uiNewMaxBlockIndex - uiNewTagBlockStart) + 1;

  // Early out for non-filled tag sets
  if (m_uiTagBlockStart == 0xFFFFFFFFu)
  {
    m_uiTagBlockStart = uiNewTagBlockStart;
    m_TagBlocks.SetCount(uiNewBlockArraySize);

    return;
  }

  EZ_ASSERT_DEBUG(uiNewTagBlockStart <= m_uiTagBlockStart, "New block start must be smaller or equal to current block start!");

  ezHybridArray<ezUInt64, 32, BlockStorageAllocator> helperArray;

  helperArray.SetCount(uiNewBlockArraySize);

  const ezUInt32 uiOldBlockStartOffset = m_uiTagBlockStart - uiNewTagBlockStart;

  // Copy old data to the new array
  ezMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  m_uiTagBlockStart = uiNewTagBlockStart;
  m_TagBlocks = helperArray;
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
void ezTagSetTemplate<BlockStorageAllocator>::Save(ezStreamWriter& stream) const
{
  const ezUInt32 uiNumTags = GetNumTagsSet();
  stream << uiNumTags;

  for (Iterator it = GetIterator(); it.IsValid(); ++it)
  {
    const ezTag* pTag = *it;

    const ezUInt32 uiTagHash = pTag->GetTagHash();
    stream << uiTagHash;
  }
}

template <typename BlockStorageAllocator /*= ezDefaultAllocatorWrapper*/>
void ezTagSetTemplate<BlockStorageAllocator>::Load(ezStreamReader& stream, const ezTagRegistry& registry)
{
  ezUInt32 uiNumTags = 0;
  stream >> uiNumTags;

  for (ezUInt32 i = 0; i < uiNumTags; ++i)
  {
    ezUInt32 uiTagHash = 0;
    stream >> uiTagHash;

    const ezTag* pTag = registry.GetTagByName(ezTempHashedString(uiTagHash));

    if (pTag != nullptr)
      Set(*pTag);
  }
}
