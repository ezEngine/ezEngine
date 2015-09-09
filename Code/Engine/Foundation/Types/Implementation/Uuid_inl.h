
ezUuid::ezUuid()
  : m_uiHigh(0), m_uiLow(0)
{
}

bool ezUuid::operator == (const ezUuid& Other) const
{
  return m_uiHigh == Other.m_uiHigh && m_uiLow == Other.m_uiLow;
}

bool ezUuid::operator != (const ezUuid& Other) const
{
  return m_uiHigh != Other.m_uiHigh || m_uiLow != Other.m_uiLow;
}

bool ezUuid::operator < (const ezUuid& Other) const
{
  if (m_uiHigh < Other.m_uiHigh)
    return true;
  if (m_uiHigh > Other.m_uiHigh)
    return false;

  return m_uiLow < Other.m_uiLow;
}

bool ezUuid::IsValid() const
{
  return m_uiHigh != 0 || m_uiLow != 0;
}

void ezUuid::CombineWithSeed(const ezUuid& seed)
{
  m_uiHigh += seed.m_uiHigh;
  m_uiLow += seed.m_uiLow;
}

void ezUuid::RevertCombinationWithSeed(const ezUuid& seed)
{
  m_uiHigh -= seed.m_uiHigh;
  m_uiLow -= seed.m_uiLow;
}

template <>
struct ezHashHelper<ezUuid>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const ezUuid& value)
  {
    return ezHashing::MurmurHash(&value, sizeof(ezUuid));
  }

  EZ_FORCE_INLINE static bool Equal(const ezUuid& a, const ezUuid& b)
  {
    return a == b;
  }
};

