#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Tracks/ColorGradient.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorGradientColorCP, ezNoBase, 1, ezRTTIDefaultAllocator<ezColorGradientColorCP>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Red", m_GammaRed)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Green", m_GammaGreen)->AddAttributes(new ezDefaultValueAttribute(255)),
    EZ_MEMBER_PROPERTY("Blue", m_GammaBlue)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorGradientAlphaCP, ezNoBase, 1, ezRTTIDefaultAllocator<ezColorGradientAlphaCP>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new ezDefaultValueAttribute(255)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorGradientIntensityCP, ezNoBase, 1, ezRTTIDefaultAllocator<ezColorGradientIntensityCP>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Intensity", m_Intensity)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezColorGradient, ezNoBase, 1, ezRTTIDefaultAllocator<ezColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("ColorCPs", m_ColorCPs),
    EZ_ARRAY_MEMBER_PROPERTY("AlphaCPs", m_AlphaCPs),
    EZ_ARRAY_MEMBER_PROPERTY("IntensityCPs", m_IntensityCPs),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezColorGradient::ezColorGradient()
{
  Clear();
}

ezColorGradient::ezColorGradient(const ezColorGradient& rhs)
{
  m_ColorCPs = rhs.m_ColorCPs;
  m_AlphaCPs = rhs.m_AlphaCPs;
  m_IntensityCPs = rhs.m_IntensityCPs;
  // Deliberately not copying m_ColorOrder, m_AlphaOrder, m_IntensityOrder, m_InitializationMutex
  // These will be rebuilt on first evaluation in the new instance
}

ezColorGradient::ezColorGradient(ezColorGradient&& rhs) noexcept
{
  m_ColorCPs = std::move(rhs.m_ColorCPs);
  m_AlphaCPs = std::move(rhs.m_AlphaCPs);
  m_IntensityCPs = std::move(rhs.m_IntensityCPs);
  // Deliberately not moving m_ColorOrder, m_AlphaOrder, m_IntensityOrder, m_InitializationMutex
  // These will be rebuilt on first evaluation in the new instance
}

void ezColorGradient::operator=(const ezColorGradient& rhs)
{
  if (this == &rhs)
    return;

  m_ColorCPs = rhs.m_ColorCPs;
  m_AlphaCPs = rhs.m_AlphaCPs;
  m_IntensityCPs = rhs.m_IntensityCPs;
  m_ColorOrder.Clear();
  m_AlphaOrder.Clear();
  m_IntensityOrder.Clear();
  // m_InitializationMutex is not copied
}

void ezColorGradient::operator=(ezColorGradient&& rhs) noexcept
{
  if (this == &rhs)
    return;

  m_ColorCPs = std::move(rhs.m_ColorCPs);
  m_AlphaCPs = std::move(rhs.m_AlphaCPs);
  m_IntensityCPs = std::move(rhs.m_IntensityCPs);
  m_ColorOrder.Clear();
  m_AlphaOrder.Clear();
  m_IntensityOrder.Clear();
  // m_InitializationMutex is not moved
}

void ezColorGradient::Clear()
{
  m_ColorCPs.Clear();
  m_AlphaCPs.Clear();
  m_IntensityCPs.Clear();
}


bool ezColorGradient::IsEmpty() const
{
  return m_ColorCPs.IsEmpty() && m_AlphaCPs.IsEmpty() && m_IntensityCPs.IsEmpty();
}

void ezColorGradient::AddColorControlPoint(double x, const ezColorGammaUB& rgb)
{
  auto& cp = m_ColorCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_GammaRed = rgb.r;
  cp.m_GammaGreen = rgb.g;
  cp.m_GammaBlue = rgb.b;
}

void ezColorGradient::AddAlphaControlPoint(double x, ezUInt8 uiAlpha)
{
  auto& cp = m_AlphaCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_Alpha = uiAlpha;
}

void ezColorGradient::AddIntensityControlPoint(double x, float fIntensity)
{
  auto& cp = m_IntensityCPs.ExpandAndGetRef();
  cp.m_iTick = TimeToTick(x);
  cp.m_Intensity = fIntensity;
}

bool ezColorGradient::GetExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  ezInt64 minTick = ezMath::MaxValue<ezInt64>();
  ezInt64 maxTick = ezMath::MinValue<ezInt64>();

  for (const auto& cp : m_ColorCPs)
  {
    minTick = ezMath::Min(minTick, cp.m_iTick);
    maxTick = ezMath::Max(maxTick, cp.m_iTick);
  }

  for (const auto& cp : m_AlphaCPs)
  {
    minTick = ezMath::Min(minTick, cp.m_iTick);
    maxTick = ezMath::Max(maxTick, cp.m_iTick);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    minTick = ezMath::Min(minTick, cp.m_iTick);
    maxTick = ezMath::Max(maxTick, cp.m_iTick);
  }

  if (minTick <= maxTick)
  {
    ref_fMinx = TickToTime(minTick);
    ref_fMaxx = TickToTime(maxTick);
    return true;
  }

  return false;
}

void ezColorGradient::GetNumControlPoints(ezUInt32& ref_uiRgb, ezUInt32& ref_uiAlpha, ezUInt32& ref_uiIntensity) const
{
  ref_uiRgb = m_ColorCPs.GetCount();
  ref_uiAlpha = m_AlphaCPs.GetCount();
  ref_uiIntensity = m_IntensityCPs.GetCount();
}


void ezColorGradient::UpdatePointOrder() const
{
  // Create remapping arrays instead of sorting the actual arrays.
  // This preserves indices so editing operations in the UI don't break when control points are moved.

  m_ColorOrder.SetCount((ezUInt16)m_ColorCPs.GetCount());
  for (ezUInt8 i = 0; i < m_ColorCPs.GetCount(); ++i)
  {
    m_ColorOrder[i] = static_cast<ezUInt8>(i);
  }
  m_ColorOrder.Sort([this](ezUInt32 a, ezUInt32 b)
    { return m_ColorCPs[a] < m_ColorCPs[b]; });

  // Alpha CPs
  m_AlphaOrder.SetCount((ezUInt16)m_AlphaCPs.GetCount());
  for (ezUInt8 i = 0; i < m_AlphaCPs.GetCount(); ++i)
  {
    m_AlphaOrder[i] = static_cast<ezUInt8>(i);
  }
  m_AlphaOrder.Sort([this](ezUInt32 a, ezUInt32 b)
    { return m_AlphaCPs[a] < m_AlphaCPs[b]; });

  // Intensity CPs
  m_IntensityOrder.SetCount((ezUInt16)m_IntensityCPs.GetCount());
  for (ezUInt8 i = 0; i < m_IntensityCPs.GetCount(); ++i)
  {
    m_IntensityOrder[i] = static_cast<ezUInt8>(i);
  }
  m_IntensityOrder.Sort([this](ezUInt32 a, ezUInt32 b)
    { return m_IntensityCPs[a] < m_IntensityCPs[b]; });

  PrecomputeLerpNormalizer();
}

void ezColorGradient::PrecomputeLerpNormalizer() const
{
  for (ezUInt32 i = 1; i < m_ColorOrder.GetCount(); ++i)
  {
    const ezUInt32 idx0 = m_ColorOrder[i - 1];
    const ezUInt32 idx1 = m_ColorOrder[i];

    const ezInt64 tick0 = m_ColorCPs[idx0].m_iTick;
    const ezInt64 tick1 = m_ColorCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_ColorCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }

  for (ezUInt32 i = 1; i < m_AlphaOrder.GetCount(); ++i)
  {
    const ezUInt32 idx0 = m_AlphaOrder[i - 1];
    const ezUInt32 idx1 = m_AlphaOrder[i];

    const ezInt64 tick0 = m_AlphaCPs[idx0].m_iTick;
    const ezInt64 tick1 = m_AlphaCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_AlphaCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }

  for (ezUInt32 i = 1; i < m_IntensityOrder.GetCount(); ++i)
  {
    const ezUInt32 idx0 = m_IntensityOrder[i - 1];
    const ezUInt32 idx1 = m_IntensityOrder[i];

    const ezInt64 tick0 = m_IntensityCPs[idx0].m_iTick;
    const ezInt64 tick1 = m_IntensityCPs[idx1].m_iTick;

    const double dist = TickToTime(tick1 - tick0);
    const double invDist = 1.0 / dist;

    m_IntensityCPs[idx0].m_fInvDistToNextCp = (float)invDist;
  }
}

void ezColorGradient::Evaluate(double x, ezColorGammaUB& ref_rgba, float& ref_fIntensity) const
{
  ref_rgba.r = 255;
  ref_rgba.g = 255;
  ref_rgba.b = 255;
  ref_rgba.a = 255;
  ref_fIntensity = 1.0f;

  EvaluateColor(x, ref_rgba);
  EvaluateAlpha(x, ref_rgba.a);
  EvaluateIntensity(x, ref_fIntensity);
}


void ezColorGradient::Evaluate(double x, ezColor& ref_hdr) const
{
  float intensity = 1.0f;
  ezUInt8 alpha = 255;

  EvaluateColor(x, ref_hdr);
  EvaluateAlpha(x, alpha);
  EvaluateIntensity(x, intensity);

  ref_hdr.ScaleRGB(intensity);
  ref_hdr.a = ezMath::ColorByteToFloat(alpha);
}

void ezColorGradient::EvaluateColor(double x, ezColorGammaUB& ref_rgb) const
{
  ezColor hdr;
  EvaluateColor(x, hdr);

  ref_rgb = hdr;
  ref_rgb.a = 255;
}

void ezColorGradient::EvaluateColor(double x, ezColor& ref_rgb) const
{
  if (m_ColorCPs.GetCount() != m_ColorOrder.GetCount())
  {
    EZ_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_ColorCPs.GetCount() != m_ColorOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_rgb.r = 1.0f;
  ref_rgb.g = 1.0f;
  ref_rgb.b = 1.0f;
  ref_rgb.a = 1.0f;

  const ezUInt32 numCPs = m_ColorCPs.GetCount();

  if (numCPs >= 2)
  {
    const ezInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping to access first CP in sorted order
    const ezUInt32 firstIdx = m_ColorOrder[0];
    if (m_ColorCPs[firstIdx].m_iTick >= xTick)
    {
      const ColorCP& cp = m_ColorCPs[firstIdx];
      ref_rgb = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

    ezUInt32 uiControlPoint;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      const ezUInt32 idx = m_ColorOrder[i];
      if (m_ColorCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const ezUInt32 lastIdx = m_ColorOrder[numCPs - 1];
      const ColorCP& cp = m_ColorCPs[lastIdx];
      ref_rgb = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

  found:
  {
    const ezUInt32 idxL = m_ColorOrder[uiControlPoint];
    const ezUInt32 idxR = m_ColorOrder[uiControlPoint + 1];

    const ColorCP& cpl = m_ColorCPs[idxL];
    const ColorCP& cpr = m_ColorCPs[idxR];

    const ezColor lhs(ezColorGammaUB(cpl.m_GammaRed, cpl.m_GammaGreen, cpl.m_GammaBlue, 255));
    const ezColor rhs(ezColorGammaUB(cpr.m_GammaRed, cpr.m_GammaGreen, cpr.m_GammaBlue, 255));

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = ezMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_rgb = ezMath::Lerp(lhs, rhs, lerpX);
  }
  }
  else if (m_ColorCPs.GetCount() == 1)
  {
    ref_rgb = ezColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
  }
}

void ezColorGradient::EvaluateAlpha(double x, ezUInt8& ref_uiAlpha) const
{
  if (m_AlphaCPs.GetCount() != m_AlphaOrder.GetCount())
  {
    EZ_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_AlphaCPs.GetCount() != m_AlphaOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_uiAlpha = 255;

  const ezUInt32 numCPs = m_AlphaCPs.GetCount();
  if (numCPs >= 2)
  {
    const ezInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping
    const ezUInt32 firstIdx = m_AlphaOrder[0];
    if (m_AlphaCPs[firstIdx].m_iTick >= xTick)
    {
      ref_uiAlpha = m_AlphaCPs[firstIdx].m_Alpha;
      return;
    }

    ezUInt32 uiControlPoint;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      const ezUInt32 idx = m_AlphaOrder[i];
      if (m_AlphaCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const ezUInt32 lastIdx = m_AlphaOrder[numCPs - 1];
      ref_uiAlpha = m_AlphaCPs[lastIdx].m_Alpha;
      return;
    }

  found:
  {
    /// \todo Use a midpoint interpolation

    const ezUInt32 idxL = m_AlphaOrder[uiControlPoint];
    const ezUInt32 idxR = m_AlphaOrder[uiControlPoint + 1];

    const AlphaCP& cpl = m_AlphaCPs[idxL];
    const AlphaCP& cpr = m_AlphaCPs[idxR];

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = ezMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_uiAlpha = ezMath::Lerp(cpl.m_Alpha, cpr.m_Alpha, lerpX);
  }
  }
  else if (m_AlphaCPs.GetCount() == 1)
  {
    ref_uiAlpha = m_AlphaCPs[0].m_Alpha;
  }
}

void ezColorGradient::EvaluateIntensity(double x, float& ref_fIntensity) const
{
  if (m_IntensityCPs.GetCount() != m_IntensityOrder.GetCount())
  {
    EZ_LOCK(m_InitializationMutex);
    // Double-check after acquiring lock
    if (m_IntensityCPs.GetCount() != m_IntensityOrder.GetCount())
    {
      UpdatePointOrder();
    }
  }

  ref_fIntensity = 1.0f;

  const ezUInt32 numCPs = m_IntensityCPs.GetCount();
  if (m_IntensityCPs.GetCount() >= 2)
  {
    const ezInt64 xTick = TimeToTick(x);

    // clamp to left value - use remapping
    const ezUInt32 firstIdx = m_IntensityOrder[0];
    if (m_IntensityCPs[firstIdx].m_iTick >= xTick)
    {
      ref_fIntensity = m_IntensityCPs[firstIdx].m_Intensity;
      return;
    }

    ezUInt32 uiControlPoint = 0;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      const ezUInt32 idx = m_IntensityOrder[i];
      if (m_IntensityCPs[idx].m_iTick >= xTick)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const ezUInt32 lastIdx = m_IntensityOrder[numCPs - 1];
      ref_fIntensity = m_IntensityCPs[lastIdx].m_Intensity;
      return;
    }

  found:
  {
    const ezUInt32 idxL = m_IntensityOrder[uiControlPoint];
    const ezUInt32 idxR = m_IntensityOrder[uiControlPoint + 1];

    const IntensityCP& cpl = m_IntensityCPs[idxL];
    const IntensityCP& cpr = m_IntensityCPs[idxR];

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const double lhsTime = TickToTime(cpl.m_iTick);
    const float lerpX = ezMath::Saturate((float)(x - lhsTime) * cpl.m_fInvDistToNextCp);

    ref_fIntensity = ezMath::Lerp(cpl.m_Intensity, cpr.m_Intensity, lerpX);
  }
  }
  else if (m_IntensityCPs.GetCount() == 1)
  {
    ref_fIntensity = m_IntensityCPs[0].m_Intensity;
  }
}

ezUInt64 ezColorGradient::GetHeapMemoryUsage() const
{
  return m_ColorCPs.GetHeapMemoryUsage() + m_AlphaCPs.GetHeapMemoryUsage() + m_IntensityCPs.GetHeapMemoryUsage();
}

void ezColorGradient::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 3;

  inout_stream << uiVersion;

  const ezUInt32 numColor = m_ColorCPs.GetCount();
  const ezUInt32 numAlpha = m_AlphaCPs.GetCount();
  const ezUInt32 numIntensity = m_IntensityCPs.GetCount();

  inout_stream << numColor;
  inout_stream << numAlpha;
  inout_stream << numIntensity;

  for (const auto& cp : m_ColorCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_GammaRed;
    inout_stream << cp.m_GammaGreen;
    inout_stream << cp.m_GammaBlue;
  }

  for (const auto& cp : m_AlphaCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_Alpha;
  }

  for (const auto& cp : m_IntensityCPs)
  {
    inout_stream << cp.m_iTick;
    inout_stream << cp.m_Intensity;
  }
}

void ezColorGradient::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 3, "Incorrect version '{0}' for ezColorGradient", uiVersion);

  ezUInt32 numColor = 0;
  ezUInt32 numAlpha = 0;
  ezUInt32 numIntensity = 0;

  inout_stream >> numColor;
  inout_stream >> numAlpha;
  inout_stream >> numIntensity;

  m_ColorCPs.SetCountUninitialized((ezUInt16)numColor);
  m_AlphaCPs.SetCountUninitialized((ezUInt16)numAlpha);
  m_IntensityCPs.SetCountUninitialized((ezUInt16)numIntensity);

  if (uiVersion == 1)
  {
    // Version 1: float positions
    float x;
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Intensity;
    }
  }
  else if (uiVersion == 2)
  {
    // Version 2: double positions
    double x;
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> x;
      cp.m_iTick = TimeToTick(x);
      inout_stream >> cp.m_Intensity;
    }
  }
  else // version 3
  {
    // Version 3: tick positions
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> cp.m_iTick;
      inout_stream >> cp.m_Intensity;
    }
  }
}


ezInt64 ezColorGradient::SnapTimeToTick(double fTimeInSeconds, ezUInt32 uiFramesPerSecond)
{
  return SnapTickTo(TimeToTick(fTimeInSeconds), uiFramesPerSecond);
}

ezInt64 ezColorGradient::SnapTickTo(ezInt64 iTick, ezUInt32 uiFramesPerSecond)
{
  const ezUInt32 uiTicksPerStep = 4800 / uiFramesPerSecond;
  return static_cast<ezInt64>(ezMath::RoundToMultiple(static_cast<double>(iTick), static_cast<double>(uiTicksPerStep)));
}

double ezColorGradient::SnapTimeTo(double fTimeInSeconds, ezUInt32 uiFramesPerSecond)
{
  return TickToTime(SnapTimeToTick(fTimeInSeconds, uiFramesPerSecond));
}


EZ_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_ColorGradient);
