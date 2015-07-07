
#pragma once

#include <Foundation/Types/Tag.h>

template<typename BlockStorageAllocator>
ezTagSetTemplate<BlockStorageAllocator>::ezTagSetTemplate()
  : m_uiTagBlockStart(0xFFFFFFFFu)
{
}

template<typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Set(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (!IsTagInAllocatedRange(Tag))
  {
    const ezUInt32 uiNewBlockStart = (m_uiTagBlockStart != 0xFFFFFFFFu) ? ezMath::Min(Tag.m_uiBlockIndex, m_uiTagBlockStart) : Tag.m_uiBlockIndex;
    const ezUInt32 uiNewBlockIndex = (m_uiTagBlockStart != 0xFFFFFFFFu) ? ezMath::Max(Tag.m_uiBlockIndex, m_uiTagBlockStart) : Tag.m_uiBlockIndex;

    Reallocate(uiNewBlockStart, uiNewBlockIndex);
  }

  m_TagBlocks[Tag.m_uiBlockIndex - m_uiTagBlockStart] |= Tag.m_uiPreshiftedBit;
}

template<typename BlockStorageAllocator>
void ezTagSetTemplate<BlockStorageAllocator>::Clear(const ezTag& Tag)
{
  EZ_ASSERT_DEV(Tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(Tag))
  {
    m_TagBlocks[Tag.m_uiBlockIndex - m_uiTagBlockStart] &= (~Tag.m_uiPreshiftedBit);
  }
}

template<typename BlockStorageAllocator>
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

template<typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  return m_uiTagBlockStart == 0xFFFFFFFFu;
}

template<typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsAnySet(const ezTagSetTemplate& OtherSet) const
{
  // If any of the sets is empty nothing can match
  if (m_uiTagBlockStart == 0xFFFFFFFFu || OtherSet.m_uiTagBlockStart == 0xFFFFFFFFu)
    return false;

  // Calculate range to compare
  const ezUInt32 uiMaxBlockStart = ezMath::Max(m_uiTagBlockStart, OtherSet.m_uiTagBlockStart);
  const ezUInt32 uiMinBlockEnd = ezMath::Min(m_uiTagBlockStart + m_TagBlocks.GetCount(), OtherSet.m_uiTagBlockStart + OtherSet.m_TagBlocks.GetCount());

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

template<typename BlockStorageAllocator>
bool ezTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const ezTag& Tag) const
{
  return Tag.m_uiBlockIndex >= m_uiTagBlockStart && Tag.m_uiBlockIndex < (m_uiTagBlockStart + m_TagBlocks.GetCount());
}

template<typename BlockStorageAllocator>
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

  if (uiOldBlockStartOffset > 0)
  {
    ezMemoryUtils::ZeroFill(helperArray.GetData(), uiOldBlockStartOffset);
  }

  // Copy old data to the new array
  ezMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  m_uiTagBlockStart = uiNewTagBlockStart;
  m_TagBlocks = helperArray;
}