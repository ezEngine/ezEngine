#include <Foundation/PCH.h>
#include <Foundation/Math/Random.h>


ezRandom::ezRandom()
{
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_uiState); ++i)
    m_uiState[i] = 0;

  m_uiIndex = 0xFFFFFFFF;
}

void ezRandom::Initialize(ezUInt64 uiSeed)
{
  m_uiIndex = 0;

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_uiState); i += 2)
  {
    m_uiState[i + 0] = uiSeed & 0xFFFFFFFF;
    m_uiState[i + 1] = (uiSeed >> 32) & 0xFFFFFFFF;
  }

  // skip the first values to ensure the random number generator is 'warmed up'
  for (ezUInt32 i = 0; i < 128; ++i)
  {
    UInt();
  }
}


void ezRandom::Save(ezStreamWriter& stream) const
{
  stream << m_uiIndex;

  stream.WriteBytes(&m_uiState[0], sizeof(ezUInt32) * 16);
}


void ezRandom::Load(ezStreamReader& stream)
{
  stream >> m_uiIndex;

  stream.ReadBytes(&m_uiState[0], sizeof(ezUInt32) * 16);
}

ezUInt32 ezRandom::UInt()
{
  EZ_ASSERT_DEBUG(m_uiIndex < 16, "Random number generator has not been initialized");

  // Implementation for the random number generator was copied from here:
  // http://stackoverflow.com/questions/1046714/what-is-a-good-random-number-generator-for-a-game
  //
  // It is the WELL algorithm from this paper:
  // http://www.lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf

  ezUInt32 a, b, c, d;
  a = m_uiState[m_uiIndex];
  c = m_uiState[(m_uiIndex + 13) & 15];
  b = (a ^ c) ^ (a << 16) ^ (c << 15);
  c = m_uiState[(m_uiIndex + 9) & 15];
  c ^= (c >> 11);
  a = m_uiState[m_uiIndex] = b ^ c;
  d = a ^ ((a << 5) & 0xDA442D24UL);
  m_uiIndex = (m_uiIndex + 15) & 15;
  a = m_uiState[m_uiIndex];
  m_uiState[m_uiIndex] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
  return m_uiState[m_uiIndex];
}

ezUInt32 ezRandom::UIntInRange(ezUInt32 uiRange)
{
  return (ezUInt32)((DoubleZeroToOneExclusive() * (double)uiRange));
}

ezInt32 ezRandom::IntInRange(ezInt32 iMinValue, ezUInt32 uiRange)
{
  return iMinValue + (ezInt32)((DoubleZeroToOneExclusive() * (double)uiRange));
}

ezInt32 ezRandom::IntMinMax(ezInt32 iMinValue, ezInt32 iMaxValue)
{
  EZ_ASSERT_DEBUG(iMinValue <= iMaxValue, "Invalid min/max values");

  return iMinValue + (ezUInt32)((DoubleZeroToOneExclusive() * (double)(iMaxValue - iMinValue + 1)));
}

double ezRandom::DoubleInRange(double fMinValue, double fRange)
{
  return fMinValue + DoubleZeroToOneExclusive() * fRange;
}

double ezRandom::DoubleMinMax(double fMinValue, double fMaxValue)
{
  EZ_ASSERT_DEBUG(fMinValue <= fMaxValue, "Invalid min/max values");

  return fMinValue + DoubleZeroToOneExclusive() * (fMaxValue - fMinValue); /// \todo Probably not correct

}
