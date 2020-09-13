
EZ_ALWAYS_INLINE ezRational::ezRational()
  : m_uiNumerator(0)
  , m_uiDenominator(1)
{}

EZ_ALWAYS_INLINE ezRational::ezRational(ezUInt32 uiNumerator, ezUInt32 uiDenominator)
  : m_uiNumerator(uiNumerator)
  , m_uiDenominator(uiDenominator)
{
}

EZ_ALWAYS_INLINE bool ezRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

EZ_ALWAYS_INLINE bool ezRational::operator==(const ezRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

EZ_ALWAYS_INLINE bool ezRational::operator!=(const ezRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

EZ_ALWAYS_INLINE ezUInt32 ezRational::GetNumerator() const
{
  return m_uiNumerator;
}

EZ_ALWAYS_INLINE ezUInt32 ezRational::GetDenominator() const
{
  return m_uiDenominator;
}

EZ_ALWAYS_INLINE ezUInt32 ezRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

EZ_ALWAYS_INLINE double ezRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);
}

EZ_ALWAYS_INLINE bool ezRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

EZ_ALWAYS_INLINE ezRational ezRational::ReduceIntegralFraction() const
{
  EZ_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return ezRational(m_uiNumerator / m_uiDenominator, 1);
}
