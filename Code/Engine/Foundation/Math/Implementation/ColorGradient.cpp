#include <Foundation/PCH.h>
#include <Foundation/Math/ColorGradient.h>
#include <Foundation/IO/Stream.h>

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

void ezColorGradient::AddColorControlPoint(float x, const ezColorGammaUB& rgb)
{
  auto& cp = m_ColorCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_GammaRed = rgb.r;
  cp.m_GammaGreen = rgb.g;
  cp.m_GammaBlue = rgb.b;
}

void ezColorGradient::AddAlphaControlPoint(float x, ezUInt8 alpha)
{
  auto& cp = m_AlphaCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Alpha = alpha;
}

void ezColorGradient::AddIntensityControlPoint(float x, float intensity)
{
  auto& cp = m_IntensityCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Intensity = intensity;
}

bool ezColorGradient::GetExtents(float& minx, float& maxx) const
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
}

void ezColorGradient::Evaluate(float x, ezColorGammaUB& rgba, float& intensity) const
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


void ezColorGradient::Evaluate(float x, ezColor& hdr) const
{
  float intensity = 1.0f;
  ezUInt8 alpha = 255;

  EvaluateColor(x, hdr);
  EvaluateAlpha(x, alpha);
  EvaluateIntensity(x, intensity);

  hdr.ScaleRGB(intensity);
  hdr.a = ezMath::ColorByteToFloat(alpha);
}

void ezColorGradient::EvaluateColor(float x, ezColorGammaUB& rgb) const
{
  ezColor hdr;
  EvaluateColor(x, hdr);

  rgb = hdr;
  rgb.a = 255;
}

void ezColorGradient::EvaluateColor(float x, ezColor& rgb) const
{
  rgb.r = 1.0f;
  rgb.g = 1.0f;
  rgb.b = 1.0f;
  rgb.a = 1.0f;

  if (m_ColorCPs.GetCount() == 1)
  {
    rgb = ezColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
  }
  else if (m_ColorCPs.GetCount() >= 2)
  {
    ezInt32 iControlPoint = -1;

    const ezUInt32 numCPs = m_ColorCPs.GetCount();
    for (ezUInt32 i = 0; i < numCPs; ++i)
    {
      if (m_ColorCPs[i].m_PosX >= x)
        break;

      iControlPoint = i;
    }

    if (iControlPoint == -1)
    {
      // clamp to left value
      rgb = ezColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      rgb = ezColorGammaUB(m_ColorCPs[numCPs - 1].m_GammaRed, m_ColorCPs[numCPs - 1].m_GammaGreen, m_ColorCPs[numCPs - 1].m_GammaBlue);
    }
    else
    {
      const ezColor lhs(ezColorGammaUB(m_ColorCPs[iControlPoint].m_GammaRed, m_ColorCPs[iControlPoint].m_GammaGreen, m_ColorCPs[iControlPoint].m_GammaBlue, 255));
      const ezColor rhs(ezColorGammaUB(m_ColorCPs[iControlPoint + 1].m_GammaRed, m_ColorCPs[iControlPoint + 1].m_GammaGreen, m_ColorCPs[iControlPoint + 1].m_GammaBlue, 255));

      /// \todo Use a midpoint interpolation

      // interpolate (linear for now)
      float lerpX = x - m_ColorCPs[iControlPoint].m_PosX;
      lerpX /= (m_ColorCPs[iControlPoint + 1].m_PosX - m_ColorCPs[iControlPoint].m_PosX);

      rgb = ezMath::Lerp(lhs, rhs, lerpX);
    }
  }
}

void ezColorGradient::EvaluateAlpha(float x, ezUInt8& alpha) const
{
  alpha = 255;

  if (m_AlphaCPs.GetCount() == 1)
  {
    alpha = m_AlphaCPs[0].m_Alpha;
  }
  else if (m_AlphaCPs.GetCount() >= 2)
  {
    ezInt32 iControlPoint = -1;

    const ezUInt32 numCPs = m_AlphaCPs.GetCount();
    for (ezUInt32 i = 0; i < numCPs; ++i)
    {
      if (m_AlphaCPs[i].m_PosX >= x)
        break;

      iControlPoint = i;
    }

    if (iControlPoint == -1)
    {
      // clamp to left value
      alpha = m_AlphaCPs[0].m_Alpha;
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      alpha = m_AlphaCPs[numCPs - 1].m_Alpha;
    }
    else
    {
      /// \todo Use a midpoint interpolation

      // interpolate (linear for now)
      float lerpX = x - m_AlphaCPs[iControlPoint].m_PosX;
      lerpX /= (m_AlphaCPs[iControlPoint + 1].m_PosX - m_AlphaCPs[iControlPoint].m_PosX);

      alpha = ezMath::Lerp(m_AlphaCPs[iControlPoint].m_Alpha, m_AlphaCPs[iControlPoint + 1].m_Alpha, lerpX);
    }
  }
}

void ezColorGradient::EvaluateIntensity(float x, float& intensity) const
{
  intensity = 1.0f;

  if (m_IntensityCPs.GetCount() == 1)
  {
    intensity = m_IntensityCPs[0].m_Intensity;
  }
  else if (m_IntensityCPs.GetCount() >= 2)
  {
    ezInt32 iControlPoint = -1;

    const ezUInt32 numCPs = m_IntensityCPs.GetCount();
    for (ezUInt32 i = 0; i < numCPs; ++i)
    {
      if (m_IntensityCPs[i].m_PosX >= x)
        break;

      iControlPoint = i;
    }

    if (iControlPoint == -1)
    {
      // clamp to left value
      intensity = m_IntensityCPs[0].m_Intensity;
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      intensity = m_IntensityCPs[numCPs - 1].m_Intensity;
    }
    else
    {
      /// \todo Use a midpoint interpolation

      // interpolate (linear for now)
      float lerpX = x - m_IntensityCPs[iControlPoint].m_PosX;
      lerpX /= (m_IntensityCPs[iControlPoint + 1].m_PosX - m_IntensityCPs[iControlPoint].m_PosX);

      intensity = ezMath::Lerp(m_IntensityCPs[iControlPoint].m_Intensity, m_IntensityCPs[iControlPoint + 1].m_Intensity, lerpX);
    }
  }
}

ezUInt64 ezColorGradient::GetHeapMemoryUsage() const
{
  return m_ColorCPs.GetHeapMemoryUsage() + m_AlphaCPs.GetHeapMemoryUsage() + m_IntensityCPs.GetHeapMemoryUsage();
}

void ezColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;

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
  EZ_ASSERT_DEV(uiVersion == 1, "Incorrect version '{0}' for ezColorGradient", uiVersion);

  ezUInt32 numColor = 0;
  ezUInt32 numAlpha = 0;
  ezUInt32 numIntensity = 0;

  stream >> numColor;
  stream >> numAlpha;
  stream >> numIntensity;

  m_ColorCPs.SetCountUninitialized(numColor);
  m_AlphaCPs.SetCountUninitialized(numAlpha);
  m_IntensityCPs.SetCountUninitialized(numIntensity);

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



EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_ColorGradient);

