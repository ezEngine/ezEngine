
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
