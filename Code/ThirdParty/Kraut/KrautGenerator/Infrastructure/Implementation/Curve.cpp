#include <KrautGenerator/PCH.h>

#include <KrautGenerator/Infrastructure/Curve.h>

namespace Kraut
{
  void Curve::Initialize(aeUInt32 uiSamples, float fInitVal, float fMin, float fMax)
  {
    m_fMinValue = fMin;
    m_fMaxValue = fMax;
    m_Values.resize(uiSamples);

    for (aeUInt32 i = 0; i < uiSamples; ++i)
      m_Values[i] = fInitVal;
  }

  float Curve::GetValueAt(float fPosition) const
  {
    const aeUInt32 uiLastSample = (m_Values.size() - 1);
    const float fSamplePos = fPosition * uiLastSample;

    const aeInt32 iSample0 = aeMath::Clamp<aeInt32>((aeInt32)aeMath::Floor(fSamplePos), 0, uiLastSample);
    const aeInt32 iSample1 = aeMath::Clamp<aeInt32>((aeInt32)aeMath::Ceil(fSamplePos), 0, uiLastSample);

    const float fLerp = aeMath::Clamp<float>(fSamplePos - (float)iSample0, 0.0f, 1.0f);

    return aeMath::Lerp(m_Values[iSample0], m_Values[iSample1], fLerp);
  }

  float Curve::GetValueAtMirrored(float fPosition) const
  {
    fPosition *= 2.0f;

    if (fPosition > 1.0f)
      fPosition = 2.0f - fPosition;

    return GetValueAt(fPosition);
  }

  void Curve::PasteCurve(const Curve& OtherCurve)
  {
    for (aeUInt32 i = 0; i < m_Values.size(); ++i)
    {
      const float fPos = (float)i / (float)(m_Values.size() - 1);
      const float fValue = OtherCurve.GetValueAt(fPos);

      const float fRelativeValue = (fValue - OtherCurve.m_fMinValue) / (OtherCurve.m_fMaxValue - OtherCurve.m_fMinValue);

      m_Values[i] = m_fMinValue + fRelativeValue * (m_fMaxValue - m_fMinValue);
    }
  }

  void Curve::Serialize(aeStreamOut& s) const
  {
    aeUInt32 count = m_Values.size();

    aeUInt8 uiVersion = 1;

    s << uiVersion;
    s << m_fMinValue;
    s << m_fMaxValue;

    s << count;

    for (aeUInt32 i = 0; i < count; ++i)
      s << m_Values[i];
  }

  void Curve::Deserialize(aeStreamIn& s, bool bOldFormat)
  {
    aeArray<float> Values;
    aeUInt32 count;

    aeUInt8 uiVersion;

    Curve Other;

    if (!bOldFormat)
    {
      s >> uiVersion;
      s >> Other.m_fMinValue;
      s >> Other.m_fMaxValue;
    }
    else
    {
      Other.m_fMinValue = m_fMinValue;
      Other.m_fMaxValue = m_fMaxValue;
    }

    s >> count;
    Other.m_Values.resize(count);

    for (aeUInt32 i = 0; i < count; ++i)
      s >> Other.m_Values[i];

    PasteCurve(Other);
  }

} // namespace Kraut
