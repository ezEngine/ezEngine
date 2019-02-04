#include <PCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/ColorGradient.h>

ezColorGradient::ezColorGradient()
{
  Clear();
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
  cp.m_PosX = x;
  cp.m_GammaRed = rgb.r;
  cp.m_GammaGreen = rgb.g;
  cp.m_GammaBlue = rgb.b;
}

void ezColorGradient::AddAlphaControlPoint(double x, ezUInt8 alpha)
{
  auto& cp = m_AlphaCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Alpha = alpha;
}

void ezColorGradient::AddIntensityControlPoint(double x, float intensity)
{
  auto& cp = m_IntensityCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Intensity = intensity;
}

bool ezColorGradient::GetExtents(double& minx, double& maxx) const
{
  minx = ezMath::BasicType<float>::MaxValue();
  maxx = -ezMath::BasicType<float>::MaxValue();

  for (const auto& cp : m_ColorCPs)
  {
    minx = ezMath::Min(minx, cp.m_PosX);
    maxx = ezMath::Max(maxx, cp.m_PosX);
  }

  for (const auto& cp : m_AlphaCPs)
  {
    minx = ezMath::Min(minx, cp.m_PosX);
    maxx = ezMath::Max(maxx, cp.m_PosX);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    minx = ezMath::Min(minx, cp.m_PosX);
    maxx = ezMath::Max(maxx, cp.m_PosX);
  }

  return minx <= maxx;
}

void ezColorGradient::GetNumControlPoints(ezUInt32& rgb, ezUInt32& alpha, ezUInt32& intensity) const
{
  rgb = m_ColorCPs.GetCount();
  alpha = m_AlphaCPs.GetCount();
  intensity = m_IntensityCPs.GetCount();
}

void ezColorGradient::SortControlPoints()
{
  m_ColorCPs.Sort();
  m_AlphaCPs.Sort();
  m_IntensityCPs.Sort();

  PrecomputeLerpNormalizer();
}

void ezColorGradient::PrecomputeLerpNormalizer()
{
  for (ezUInt32 i = 1; i < m_ColorCPs.GetCount(); ++i)
  {
    const double px0 = m_ColorCPs[i - 1].m_PosX;
    const double px1 = m_ColorCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_ColorCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }

  for (ezUInt32 i = 1; i < m_AlphaCPs.GetCount(); ++i)
  {
    const double px0 = m_AlphaCPs[i - 1].m_PosX;
    const double px1 = m_AlphaCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_AlphaCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }

  for (ezUInt32 i = 1; i < m_IntensityCPs.GetCount(); ++i)
  {
    const double px0 = m_IntensityCPs[i - 1].m_PosX;
    const double px1 = m_IntensityCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_IntensityCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }
}

void ezColorGradient::Evaluate(double x, ezColorGammaUB& rgba, float& intensity) const
{
  rgba.r = 255;
  rgba.g = 255;
  rgba.b = 255;
  rgba.a = 255;
  intensity = 1.0f;

  EvaluateColor(x, rgba);
  EvaluateAlpha(x, rgba.a);
  EvaluateIntensity(x, intensity);
}


void ezColorGradient::Evaluate(double x, ezColor& hdr) const
{
  float intensity = 1.0f;
  ezUInt8 alpha = 255;

  EvaluateColor(x, hdr);
  EvaluateAlpha(x, alpha);
  EvaluateIntensity(x, intensity);

  hdr.ScaleRGB(intensity);
  hdr.a = ezMath::ColorByteToFloat(alpha);
}

void ezColorGradient::EvaluateColor(double x, ezColorGammaUB& rgb) const
{
  ezColor hdr;
  EvaluateColor(x, hdr);

  rgb = hdr;
  rgb.a = 255;
}

void ezColorGradient::EvaluateColor(double x, ezColor& rgb) const
{
  rgb.r = 1.0f;
  rgb.g = 1.0f;
  rgb.b = 1.0f;
  rgb.a = 1.0f;

  const ezUInt32 numCPs = m_ColorCPs.GetCount();

  if (numCPs >= 2)
  {
    // clamp to left value
    if (m_ColorCPs[0].m_PosX >= x)
    {
      const ColorCP& cp = m_ColorCPs[0];
      rgb = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

    ezUInt32 uiControlPoint;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_ColorCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const ColorCP& cp = m_ColorCPs[numCPs - 1];
      rgb = ezColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

  found:
  {
    const ColorCP& cpl = m_ColorCPs[uiControlPoint];
    const ColorCP& cpr = m_ColorCPs[uiControlPoint + 1];

    const ezColor lhs(ezColorGammaUB(cpl.m_GammaRed, cpl.m_GammaGreen, cpl.m_GammaBlue, 255));
    const ezColor rhs(ezColorGammaUB(cpr.m_GammaRed, cpr.m_GammaGreen, cpr.m_GammaBlue, 255));

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    rgb = ezMath::Lerp(lhs, rhs, lerpX);
  }
  }
  else if (m_ColorCPs.GetCount() == 1)
  {
    rgb = ezColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
  }
}

void ezColorGradient::EvaluateAlpha(double x, ezUInt8& alpha) const
{
  alpha = 255;

  const ezUInt32 numCPs = m_AlphaCPs.GetCount();
  if (numCPs >= 2)
  {
    // clamp to left value
    if (m_AlphaCPs[0].m_PosX >= x)
    {
      alpha = m_AlphaCPs[0].m_Alpha;
      return;
    }

    ezUInt32 uiControlPoint;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_AlphaCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      alpha = m_AlphaCPs[numCPs - 1].m_Alpha;
      return;
    }

  found:
  {
    /// \todo Use a midpoint interpolation

    const AlphaCP& cpl = m_AlphaCPs[uiControlPoint];
    const AlphaCP& cpr = m_AlphaCPs[uiControlPoint + 1];

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    alpha = ezMath::Lerp(cpl.m_Alpha, cpr.m_Alpha, lerpX);
  }
  }
  else if (m_AlphaCPs.GetCount() == 1)
  {
    alpha = m_AlphaCPs[0].m_Alpha;
  }
}

void ezColorGradient::EvaluateIntensity(double x, float& intensity) const
{
  intensity = 1.0f;

  const ezUInt32 numCPs = m_IntensityCPs.GetCount();
  if (m_IntensityCPs.GetCount() >= 2)
  {
    // clamp to left value
    if (m_IntensityCPs[0].m_PosX >= x)
    {
      intensity = m_IntensityCPs[0].m_Intensity;
      return;
    }

    ezUInt32 uiControlPoint;

    for (ezUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_IntensityCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      intensity = m_IntensityCPs[numCPs - 1].m_Intensity;
      return;
    }

  found:
  {
    const IntensityCP& cpl = m_IntensityCPs[uiControlPoint];
    const IntensityCP& cpr = m_IntensityCPs[uiControlPoint + 1];

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    intensity = ezMath::Lerp(cpl.m_Intensity, cpr.m_Intensity, lerpX);
  }
  }
  else if (m_IntensityCPs.GetCount() == 1)
  {
    intensity = m_IntensityCPs[0].m_Intensity;
  }
}

ezUInt64 ezColorGradient::GetHeapMemoryUsage() const
{
  return m_ColorCPs.GetHeapMemoryUsage() + m_AlphaCPs.GetHeapMemoryUsage() + m_IntensityCPs.GetHeapMemoryUsage();
}

void ezColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;

  stream << uiVersion;

  const ezUInt32 numColor = m_ColorCPs.GetCount();
  const ezUInt32 numAlpha = m_AlphaCPs.GetCount();
  const ezUInt32 numIntensity = m_IntensityCPs.GetCount();

  stream << numColor;
  stream << numAlpha;
  stream << numIntensity;

  for (const auto& cp : m_ColorCPs)
  {
    stream << cp.m_PosX;
    stream << cp.m_GammaRed;
    stream << cp.m_GammaGreen;
    stream << cp.m_GammaBlue;
  }

  for (const auto& cp : m_AlphaCPs)
  {
    stream << cp.m_PosX;
    stream << cp.m_Alpha;
  }

  for (const auto& cp : m_IntensityCPs)
  {
    stream << cp.m_PosX;
    stream << cp.m_Intensity;
  }
}

void ezColorGradient::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= 2, "Incorrect version '{0}' for ezColorGradient", uiVersion);

  ezUInt32 numColor = 0;
  ezUInt32 numAlpha = 0;
  ezUInt32 numIntensity = 0;

  stream >> numColor;
  stream >> numAlpha;
  stream >> numIntensity;

  m_ColorCPs.SetCountUninitialized(numColor);
  m_AlphaCPs.SetCountUninitialized(numAlpha);
  m_IntensityCPs.SetCountUninitialized(numIntensity);

  if (uiVersion == 1)
  {
    float x;
    for (auto& cp : m_ColorCPs)
    {
      stream >> x;
      cp.m_PosX = x; // float
      stream >> cp.m_GammaRed;
      stream >> cp.m_GammaGreen;
      stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      stream >> x;
      cp.m_PosX = x; // float
      stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      stream >> x;
      cp.m_PosX = x; // float
      stream >> cp.m_Intensity;
    }
  }
  else
  {
    for (auto& cp : m_ColorCPs)
    {
      stream >> cp.m_PosX;
      stream >> cp.m_GammaRed;
      stream >> cp.m_GammaGreen;
      stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      stream >> cp.m_PosX;
      stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      stream >> cp.m_PosX;
      stream >> cp.m_Intensity;
    }
  }

  PrecomputeLerpNormalizer();
}



EZ_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_ColorGradient);

