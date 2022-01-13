module ez.foundation.math.random;

import ez.foundation.types.types;

extern(C++, class) struct ezRandom
{
  // CODEGEN-BEGIN: Struct("ezRandom")
  void Initialize(ulong uiSeed);
  void InitializeFromCurrentTime();
  // method 'Save' - unsupported argument 'stream'
  // method 'Load' - unsupported argument 'stream'
  uint UInt();
  uint UIntInRange(uint uiRange);
  int IntInRange(int iMinValue, uint uiRange);
  int IntMinMax(int iMinValue, int iMaxValue);
  bool Bool();
  double DoubleZeroToOneExclusive();
  double DoubleZeroToOneInclusive();
  double DoubleInRange(double fMinValue, double fRange);
  double DoubleMinMax(double fMinValue, double fMaxValue);
  double DoubleVariance(double fValue, double fVariance);
  double DoubleVarianceAroundZero(double fAbsMaxValue);
  float FloatZeroToOneExclusive();
  float FloatZeroToOneInclusive();
  float FloatInRange(float fMinValue, float fRange);
  float FloatMinMax(float fMinValue, float fMaxValue);
  float FloatVariance(float fValue, float fVariance);
  float FloatVarianceAroundZero(float fAbsMaxValue);
private:
  uint m_uiIndex;
  uint[16] m_uiState;
  // Operator: =
  // CODEGEN-END
}

extern(C++) ezRandom* Make_ezRandom();