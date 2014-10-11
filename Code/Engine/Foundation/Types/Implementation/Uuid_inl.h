
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

bool ezUuid::IsValid() const
{
  return m_uiHigh != 0 || m_uiLow != 0;
}

ezUuid::ezUuid(ezUInt64 uiHigh, ezUInt64 uiLow)
  : m_uiHigh(uiHigh), m_uiLow(uiLow)
{
}
