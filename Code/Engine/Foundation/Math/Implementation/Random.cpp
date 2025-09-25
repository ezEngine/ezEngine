#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Time/Timestamp.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRandom, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(UInt)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(UIntInRange, In, "Range")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(UInt32Index, In, "ArraySize", In, "FallbackValue")->AddFlags(ezPropertyFlags::PureFunction)->AddAttributes(new ezFunctionArgumentAttributes(1, new ezDefaultValueAttribute(-1))),
    EZ_SCRIPT_FUNCTION_PROPERTY(UInt16Index, In, "ArraySize", In, "FallbackValue")->AddFlags(ezPropertyFlags::PureFunction)->AddAttributes(new ezFunctionArgumentAttributes(1, new ezDefaultValueAttribute(-1))),
    EZ_SCRIPT_FUNCTION_PROPERTY(IntMinMax, In, "MinValue", In, "MaxValue")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(Bool)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(DoubleZeroToOneExclusive)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(DoubleZeroToOneInclusive)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(DoubleMinMax, In, "MinValue", In, "MaxValue")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(DoubleVariance, In, "Value", In, "Variance")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(DoubleVarianceAroundZero, In, "AbsMaxValue")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOneExclusive)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatZeroToOneInclusive)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatMinMax, In, "MinValue", In, "MaxValue")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatVariance, In, "Value", In, "Variance")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(FloatVarianceAroundZero, In, "AbsMaxValue")->AddFlags(ezPropertyFlags::PureFunction),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezRandom::ezRandom() = default;
ezRandom::~ezRandom() = default;

void ezRandom::Initialize(ezUInt64 uiSeed)
{
  // make sure the seed is never zero
  // otherwise the state will become zero and the RNG will produce only zeros
  uiSeed ^= 0x0102030405060708llu;

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


void ezRandom::InitializeFromCurrentTime()
{
  // needed to fix quick calls to this function that would result in an identical timestamp (it's not high resolution enough for that)
  static ezAtomicInteger32 rndAdd;

  ezTimestamp ts = ezTimestamp::CurrentTimestamp();
  Initialize(static_cast<ezUInt64>(ts.GetInt64(ezSIUnitOfTime::Nanosecond)) + rndAdd.Increment());
}

void ezRandom::Save(ezStreamWriter& inout_stream) const
{
  inout_stream << m_uiIndex;

  inout_stream.WriteBytes(&m_uiState[0], sizeof(ezUInt32) * 16).IgnoreResult();
}


void ezRandom::Load(ezStreamReader& inout_stream)
{
  inout_stream >> m_uiIndex;

  inout_stream.ReadBytes(&m_uiState[0], sizeof(ezUInt32) * 16);
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
  EZ_ASSERT_DEBUG(uiRange > 0, "Invalid range for random number");

  const ezUInt32 uiSteps = 0xFFFFFFFF / uiRange;
  const ezUInt32 uiMaxValue = uiRange * uiSteps;

  ezUInt32 result = 0;

  do
  {
    result = UInt();
  } while (result > uiMaxValue);

  return result % uiRange;
}

ezUInt32 ezRandom::UInt32Index(ezUInt32 uiArraySize, ezUInt32 uiFallbackValue /*= ezInvalidIndex*/)
{
  if (uiArraySize == 0)
    return uiFallbackValue;

  return UIntInRange(uiArraySize);
}

ezUInt16 ezRandom::UInt16Index(ezUInt16 uiArraySize, ezUInt16 uiFallbackValue /*= 0xFFFF*/)
{
  if (uiArraySize == 0)
    return uiFallbackValue;

  return static_cast<ezUInt16>(UIntInRange(uiArraySize));
}

ezInt32 ezRandom::IntMinMax(ezInt32 iMinValue, ezInt32 iMaxValue)
{
  EZ_ASSERT_DEBUG(iMinValue <= iMaxValue, "Invalid min/max values");

  return iMinValue + (ezInt32)UIntInRange(iMaxValue - iMinValue + 1);
}

double ezRandom::DoubleMinMax(double fMinValue, double fMaxValue)
{
  EZ_ASSERT_DEBUG(fMinValue <= fMaxValue, "Invalid min/max values");

  return fMinValue + DoubleZeroToOneExclusive() * (fMaxValue - fMinValue); /// \todo Probably not correct
}

double ezRandom::DoubleVariance(double fValue, double fVariance)
{
  /// \todo Test whether this is actually correct

  const double dev = DoubleZeroToOneInclusive();
  const double offset = fValue * fVariance * dev;
  return DoubleMinMax(fValue - offset, fValue + offset);
}

double ezRandom::DoubleVarianceAroundZero(double fAbsMaxValue)
{
  /// \todo Test whether this is actually correct

  const double dev = DoubleZeroToOneInclusive();
  const double offset = fAbsMaxValue * dev;
  return DoubleMinMax(-offset, +offset);
}

static double Gauss(double x, double fSigma)
{
  // taken from https://en.wikipedia.org/wiki/Normal_distribution
  // mue is 0 because we want the curve to center around the origin

  // float G = (1.0f / (sqrt (2.0f * pi) * fSigma)) * exp ((-(x * x) / (2.0f * fSigma * fSigma)));

  const double sqrt2pi = 2.506628274631000502415765284811;

  const double G = (1.0 / (sqrt2pi * fSigma)) * ezMath::Exp((-(x * x) / (2.0 * fSigma * fSigma)));

  return G;
}

void ezRandomGauss::Initialize(ezUInt64 uiRandomSeed, ezUInt32 uiMaxValue, float fVariance)
{
  EZ_ASSERT_DEV(uiMaxValue >= 2, "Invalid value");

  m_Generator.Initialize(uiRandomSeed);

  SetupTable(uiMaxValue, ezMath::Sqrt(fVariance));
}


void ezRandomGauss::SetupTable(ezUInt32 uiMaxValue, float fSigma)
{
  // create half a bell curve with a fixed sigma

  const double UsefulRange = 5.0;

  m_fSigma = fSigma;
  m_GaussAreaSum.SetCountUninitialized(uiMaxValue);

  const double fBase2 = Gauss(UsefulRange, fSigma); // we clamp to zero at uiMaxValue, so we need the Gauss value there to subtract it from all other values

  m_fAreaSum = 0;

  for (ezUInt32 i = 0; i < uiMaxValue; ++i)
  {
    const double g = Gauss((UsefulRange / (uiMaxValue - 1)) * i, fSigma) - fBase2;
    m_fAreaSum += g;
    m_GaussAreaSum[i] = (float)m_fAreaSum;
  }
}

ezUInt32 ezRandomGauss::UnsignedValue()
{
  const double fRand = m_Generator.DoubleMinMax(0, m_fAreaSum);

  const ezUInt32 uiMax = m_GaussAreaSum.GetCount();
  for (ezUInt32 i = 0; i < uiMax; ++i)
  {
    if (fRand < m_GaussAreaSum[i])
      return i;
  }

  return uiMax - 1;
}

ezInt32 ezRandomGauss::SignedValue()
{
  const double fRand = m_Generator.DoubleMinMax(-m_fAreaSum, m_fAreaSum);
  const ezUInt32 uiMax = m_GaussAreaSum.GetCount();

  if (fRand >= 0.0)
  {
    for (ezUInt32 i = 0; i < uiMax; ++i)
    {
      if (fRand < m_GaussAreaSum[i])
        return i;
    }

    return uiMax - 1;
  }
  else
  {
    const double fRandAbs = (-fRand);

    for (ezUInt32 i = 0; i < uiMax - 1; ++i)
    {
      if (fRandAbs < m_GaussAreaSum[i])
        return -(ezInt32)i - 1;
    }

    return -(ezInt32)(uiMax - 1);
  }
}

void ezRandomGauss::Save(ezStreamWriter& inout_stream) const
{
  inout_stream << m_GaussAreaSum.GetCount();
  inout_stream << m_fSigma;
  m_Generator.Save(inout_stream);
}

void ezRandomGauss::Load(ezStreamReader& inout_stream)
{
  ezUInt32 uiMax = 0;
  inout_stream >> uiMax;

  float fVariance = 0.0f;
  inout_stream >> fVariance;

  SetupTable(uiMax, fVariance);

  m_Generator.Load(inout_stream);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Random);
