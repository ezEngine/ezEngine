#pragma once

template<class Container>
EZ_FORCE_INLINE ezUInt32 ezBitfield<Container>::GetBitInt(ezUInt32 uiBitIndex) const
{
  return (uiBitIndex >> 5); // div 32
}

template<class Container>
EZ_FORCE_INLINE ezUInt32 ezBitfield<Container>::GetBitMask(ezUInt32 uiBitIndex) const
{
  return 1 << (uiBitIndex & 0x1F); // modulo 32, shifted to bit position
}

template<class Container>
ezBitfield<Container>::ezBitfield() : m_uiCount(0)
{
}

template<class Container>
ezUInt32 ezBitfield<Container>::GetCount()
{
  return m_uiCount;
}

template<class Container>
void ezBitfield<Container>::SetCount(ezUInt32 uiBitCount)
{
  const ezUInt32 uiInts = (uiBitCount + 31) >> 5;
  m_Container.SetCount(uiInts);

  m_uiCount = uiBitCount;
}

template<class Container>
void ezBitfield<Container>::SetCount(ezUInt32 uiBitCount, bool bSetNew)
{
  if (m_uiCount == uiBitCount)
    return;

  const ezUInt32 uiOldBits = m_uiCount;

  SetCount(uiBitCount);

  // if there are new bits, initialize them
  if (uiBitCount > uiOldBits)
  {
    if (bSetNew)
      SetRange(uiOldBits, uiBitCount - 1);
    else
      ClearRange(uiOldBits, uiBitCount - 1);
  }
}

template<class Container>
bool ezBitfield<Container>::IsEmpty() const
{
  return m_uiCount == 0;
}

template<class Container>
void ezBitfield<Container>::Clear()
{
  m_uiCount = 0;
  m_Container.Clear();
}

template<class Container>
void ezBitfield<Container>::SetBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEV(uiBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] |= GetBitMask(uiBit);
}

template<class Container>
void ezBitfield<Container>::ClearBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEV(uiBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] &= ~GetBitMask(uiBit);
}

template<class Container>
bool ezBitfield<Container>::IsSet(ezUInt32 uiBit) const
{
  EZ_ASSERT_DEV(uiBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiBit, m_uiCount);

  return (m_Container[GetBitInt(uiBit)] & GetBitMask(uiBit)) != 0;
}

template<class Container>
void ezBitfield<Container>::ClearAllBits()
{
  for (ezUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0;
}

template<class Container>
void ezBitfield<Container>::SetAllBits()
{
  for (ezUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0xFFFFFFFF;
}

template<class Container>
void ezBitfield<Container>::SetRange(ezUInt32 uiFirstBit, ezUInt32 uiLastBit)
{
  EZ_ASSERT_DEV(uiFirstBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiFirstBit, m_uiCount);
  EZ_ASSERT_DEV(uiLastBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiLastBit, m_uiCount);

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      SetBit(i);

    return;
  }

  const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const ezUInt32 uiPrevIntBit = uiLastInt  * 32;

  // set the bits in the first int individually
  for (ezUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    SetBit(i);

  // set the bits in the ints in between with one operation
  for (ezUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0xFFFFFFFF;

  // set the bits in the last int individually
  for (ezUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    SetBit(i);
}

template<class Container>
void ezBitfield<Container>::ClearRange(ezUInt32 uiFirstBit, ezUInt32 uiLastBit)
{
  EZ_ASSERT_DEV(uiFirstBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiFirstBit, m_uiCount);
  EZ_ASSERT_DEV(uiLastBit < m_uiCount, "Cannot access bit %i, the bitfield only has %i bits.", uiLastBit, m_uiCount);

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt  = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      ClearBit(i);

    return;
  }

  const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const ezUInt32 uiPrevIntBit = uiLastInt  * 32;

  // set the bits in the first int individually
  for (ezUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    ClearBit(i);

  // set the bits in the ints in between with one operation
  for (ezUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0;

  // set the bits in the last int individually
  for (ezUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    ClearBit(i);
}

