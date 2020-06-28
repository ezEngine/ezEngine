
ezRational::ezRational(ezUInt32 uiNumerator, ezUInt32 uiDenominator)
  : m_uiNumerator(uiNumerator)
  , m_uiDenominator(uiDenominator)
{}

bool ezRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

bool ezRational::operator==(const ezRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

bool ezRational::operator!=(const ezRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

ezUInt32 ezRational::GetNumerator() const
{
  return m_uiNumerator;
}

ezUInt32 ezRational::GetDenominator() const
{
  return m_uiDenominator;
}

/// \brief Returns the result of the division as an integer.
ezUInt32 ezRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

double ezRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);

}

/// \brief Returns true if the rational is valid (follows the rules stated in the class description)
bool ezRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

/// \brief This helper returns a reduced fraction in case of an integral input.
///
/// Note that this will assert in DEV builds if this class is not integral.
ezRational ezRational::ReduceIntegralFraction() const
{
  EZ_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return ezRational(m_uiNumerator / m_uiDenominator, 1);
}
