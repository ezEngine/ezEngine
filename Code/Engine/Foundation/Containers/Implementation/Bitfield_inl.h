#pragma once

template <class Container>
EZ_ALWAYS_INLINE ezUInt32 ezBitfield<Container>::GetBitInt(ezUInt32 uiBitIndex) const
{
  return (uiBitIndex >> 5); // div 32
}

template <class Container>
EZ_ALWAYS_INLINE ezUInt32 ezBitfield<Container>::GetBitMask(ezUInt32 uiBitIndex) const
{
  return 1 << (uiBitIndex & 0x1F); // modulo 32, shifted to bit position
}

template <class Container>
EZ_ALWAYS_INLINE ezUInt32 ezBitfield<Container>::GetCount() const
{
  return m_uiCount;
}

template <class Container>
template <typename> // Second template needed so that the compiler only instantiates it when called. Needed to prevent errors with containers that do
                    // not support this.
void ezBitfield<Container>::SetCountUninitialized(ezUInt32 uiBitCount)
{
  const ezUInt32 uiInts = (uiBitCount + 31) >> 5;
  m_Container.SetCountUninitialized(uiInts);

  m_uiCount = uiBitCount;
}

template <class Container>
void ezBitfield<Container>::SetCount(ezUInt32 uiBitCount, bool bSetNew)
{
  if (m_uiCount == uiBitCount)
    return;

  const ezUInt32 uiOldBits = m_uiCount;

  SetCountUninitialized(uiBitCount);

  // if there are new bits, initialize them
  if (uiBitCount > uiOldBits)
  {
    if (bSetNew)
      SetBitRange(uiOldBits, uiBitCount - uiOldBits);
    else
      ClearBitRange(uiOldBits, uiBitCount - uiOldBits);
  }
}

template <class Container>
EZ_ALWAYS_INLINE bool ezBitfield<Container>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <class Container>
bool ezBitfield<Container>::IsAnyBitSet(ezUInt32 uiFirstBit /*= 0*/, ezUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  EZ_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const ezUInt32 uiLastBit = ezMath::Min<ezUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }
  else
  {
    const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const ezUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (ezUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }

    // check the bits in the ints in between with one operation
    for (ezUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if ((m_Container[i] & 0xFFFFFFFF) != 0)
        return true;
    }

    // check the bits in the last int individually
    for (ezUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }

  return false;
}

template <class Container>
EZ_ALWAYS_INLINE bool ezBitfield<Container>::IsNoBitSet(ezUInt32 uiFirstBit /*= 0*/, ezUInt32 uiLastBit /*= 0xFFFFFFFF*/) const
{
  return !IsAnyBitSet(uiFirstBit, uiLastBit);
}

template <class Container>
bool ezBitfield<Container>::AreAllBitsSet(ezUInt32 uiFirstBit /*= 0*/, ezUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  EZ_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const ezUInt32 uiLastBit = ezMath::Min<ezUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }
  else
  {
    const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const ezUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (ezUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }

    // check the bits in the ints in between with one operation
    for (ezUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if (m_Container[i] != 0xFFFFFFFF)
        return false;
    }

    // check the bits in the last int individually
    for (ezUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }

  return true;
}

template <class Container>
EZ_ALWAYS_INLINE void ezBitfield<Container>::Clear()
{
  m_uiCount = 0;
  m_Container.Clear();
}

template <class Container>
void ezBitfield<Container>::SetBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] |= GetBitMask(uiBit);
}

template <class Container>
void ezBitfield<Container>::ClearBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] &= ~GetBitMask(uiBit);
}

template <class Container>
bool ezBitfield<Container>::IsBitSet(ezUInt32 uiBit) const
{
  EZ_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  return (m_Container[GetBitInt(uiBit)] & GetBitMask(uiBit)) != 0;
}

template <class Container>
void ezBitfield<Container>::ClearAllBits()
{
  for (ezUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0;
}

template <class Container>
void ezBitfield<Container>::SetAllBits()
{
  for (ezUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0xFFFFFFFF;
}

template <class Container>
void ezBitfield<Container>::SetBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  EZ_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const ezUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      SetBit(i);

    return;
  }

  const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const ezUInt32 uiPrevIntBit = uiLastInt * 32;

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

template <class Container>
void ezBitfield<Container>::ClearBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  EZ_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const ezUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const ezUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const ezUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (ezUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      ClearBit(i);

    return;
  }

  const ezUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const ezUInt32 uiPrevIntBit = uiLastInt * 32;

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
EZ_ALWAYS_INLINE ezStaticBitfield<T>::ezStaticBitfield()
{
  static_assert(std::is_unsigned<T>::value, "Storage type must be unsigned");
}

template <typename T>
EZ_ALWAYS_INLINE ezStaticBitfield<T> ezStaticBitfield<T>::FromMask(StorageType bits)
{
  return ezStaticBitfield<T>(bits);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezStaticBitfield<T>::IsAnyBitSet() const
{
  return m_Storage != 0;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezStaticBitfield<T>::IsNoBitSet() const
{
  return m_Storage == 0;
}

template <typename T>
bool ezStaticBitfield<T>::AreAllBitsSet() const
{
  const T inv = ~m_Storage;
  return inv == 0;
}

template <typename T>
void ezStaticBitfield<T>::ClearBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits)
{
  EZ_ASSERT_DEBUG(uiFirstBit < GetNumBits(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetNumBits());

  for (ezUInt32 i = 0; i < uiNumBits; ++i)
  {
    const ezUInt32 uiBit = uiFirstBit + i;
    m_Storage &= ~(static_cast<T>(1u) << uiBit);
  }
}

template <typename T>
void ezStaticBitfield<T>::SetBitRange(ezUInt32 uiFirstBit, ezUInt32 uiNumBits)
{
  EZ_ASSERT_DEBUG(uiFirstBit < GetNumBits(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetNumBits());

  for (ezUInt32 i = 0; i < uiNumBits; ++i)
  {
    const ezUInt32 uiBit = uiFirstBit + i;
    m_Storage |= static_cast<T>(1u) << uiBit;
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezStaticBitfield<T>::SetAllBits()
{
  m_Storage = ezMath::MaxValue<T>(); // possible because we assert that T is unsigned
}

template <typename T>
EZ_ALWAYS_INLINE void ezStaticBitfield<T>::ClearAllBits()
{
  m_Storage = 0;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezStaticBitfield<T>::IsBitSet(ezUInt32 uiBit) const
{
  EZ_ASSERT_DEBUG(uiBit < GetNumBits(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetNumBits());

  return (m_Storage & (static_cast<T>(1u) << uiBit)) != 0;
}

template <typename T>
EZ_ALWAYS_INLINE void ezStaticBitfield<T>::ClearBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEBUG(uiBit < GetNumBits(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetNumBits());

  m_Storage &= ~(static_cast<T>(1u) << uiBit);
}

template <typename T>
EZ_ALWAYS_INLINE void ezStaticBitfield<T>::SetBit(ezUInt32 uiBit)
{
  EZ_ASSERT_DEBUG(uiBit < GetNumBits(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetNumBits());

  m_Storage |= static_cast<T>(1u) << uiBit;
}

template <typename T>
EZ_ALWAYS_INLINE void ezStaticBitfield<T>::SetValue(T value)
{
  m_Storage = value;
}

template <typename T>
EZ_ALWAYS_INLINE T ezStaticBitfield<T>::GetValue() const
{
  return m_Storage;
}
