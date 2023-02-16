#pragma once

#include <KrautFoundation/Containers/Array.h>
#include <KrautFoundation/Streams/Streams.h>
#include <KrautGenerator/KrautGeneratorDLL.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL Curve
  {
    aeArray<float> m_Values;

    float m_fMinValue;
    float m_fMaxValue;

    void Initialize(aeUInt32 uiSamples, float fInitVal, float fMin, float fMax);

    float GetValueAt(float fPosition) const;
    float GetValueAtMirrored(float fPosition) const;

    void PasteCurve(const Curve& OtherCurve);

    void Serialize(aeStreamOut& s) const;
    void Deserialize(aeStreamIn& s, bool bOldFormat = false);
  };

} // namespace Kraut
