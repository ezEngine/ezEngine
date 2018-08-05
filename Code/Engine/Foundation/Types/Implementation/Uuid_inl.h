
ezUuid::ezUuid()
    : m_uiHigh(0)
    , m_uiLow(0)
{
}

void ezUuid::SetInvalid()
{
  m_uiHigh = 0;
  m_uiLow = 0;
}

bool ezUuid::operator==(const ezUuid& Other) const
{
  return m_uiHigh == Other.m_uiHigh && m_uiLow == Other.m_uiLow;
}

bool ezUuid::operator!=(const ezUuid& Other) const
{
  return m_uiHigh != Other.m_uiHigh || m_uiLow != Other.m_uiLow;
}

bool ezUuid::operator<(const ezUuid& Other) const
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

void ezUuid::HashCombine(const ezUuid& guid)
{
  m_uiHigh = ezHashing::xxHash64(&guid.m_uiHigh, sizeof(ezUInt64), m_uiHigh);
  m_uiLow = ezHashing::xxHash64(&guid.m_uiLow, sizeof(ezUInt64), m_uiLow);
}

template <>
struct ezHashHelper<ezUuid>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezUuid& value) { return ezHashing::xxHash32(&value, sizeof(ezUuid)); }

  EZ_ALWAYS_INLINE static bool Equal(const ezUuid& a, const ezUuid& b) { return a == b; }
};
