#include <FoundationPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

ezFloat16::ezFloat16(float f)
{
  operator=(f);
}

void ezFloat16::operator=(float f)
{
  // source: http://www.ogre3d.org/docs/api/html/OgreBitwise_8h_source.html

  const ezUInt32 i = *reinterpret_cast<ezUInt32*>(&f);

  const ezUInt32 s = (i >> 16) & 0x00008000;
  const ezInt32 e = ((i >> 23) & 0x000000ff) - (127 - 15);
  ezUInt32 m = i & 0x007fffff;

  if (e <= 0)
  {
    if (e < -10)
    {
      m_uiData = 0;
      return;
    }
    m = (m | 0x00800000) >> (1 - e);

    m_uiData = static_cast<ezUInt16>(s | (m >> 13));
  }
  else if (e == 0xff - (127 - 15))
  {
    if (m == 0) // Inf
    {
      m_uiData = static_cast<ezUInt16>(s | 0x7c00);
    }
    else // NAN
    {
      m >>= 13;
      m_uiData = static_cast<ezUInt16>(s | 0x7c00 | m | (m == 0));
    }
  }
  else
  {
    if (e > 30) // Overflow
    {
      m_uiData = static_cast<ezUInt16>(s | 0x7c00);
      return;
    }

    m_uiData = static_cast<ezUInt16>(s | (e << 10) | (m >> 13));
  }
}

ezFloat16::operator float() const
{
  const ezUInt32 s = (m_uiData >> 15) & 0x00000001;
  ezUInt32 e = (m_uiData >> 10) & 0x0000001f;
  ezUInt32 m = m_uiData & 0x000003ff;

  ezUInt32 uiResult;

  if (e == 0)
  {
    if (m == 0) // Plus or minus zero
    {
      uiResult = s << 31;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // Denormalized number -- renormalize it
    {
      while (!(m & 0x00000400))
      {
        m <<= 1;
        e -= 1;
      }

      e += 1;
      m &= ~0x00000400;
    }
  }
  else if (e == 31)
  {
    if (m == 0) // Inf
    {
      uiResult = (s << 31) | 0x7f800000;
      return *reinterpret_cast<float*>(&uiResult);
    }
    else // NaN
    {
      uiResult = (s << 31) | 0x7f800000 | (m << 13);
      return *reinterpret_cast<float*>(&uiResult);
    }
  }

  e = e + (127 - 15);
  m = m << 13;

  uiResult = (s << 31) | (e << 23) | m;

  return *reinterpret_cast<float*>(&uiResult);
}

//////////////////////////////////////////////////////////////////////////

ezFloat16Vec2::ezFloat16Vec2(const ezVec2& vec)
{
  operator=(vec);
}

void ezFloat16Vec2::operator=(const ezVec2& vec)
{
  x = vec.x;
  y = vec.y;
}

ezFloat16Vec2::operator ezVec2() const
{
  return ezVec2(x, y);
}

//////////////////////////////////////////////////////////////////////////

ezFloat16Vec3::ezFloat16Vec3(const ezVec3& vec)
{
  operator=(vec);
}

void ezFloat16Vec3::operator=(const ezVec3& vec)
{
  x = vec.x;
  y = vec.y;
  z = vec.z;
}

ezFloat16Vec3::operator ezVec3() const
{
  return ezVec3(x, y, z);
}

//////////////////////////////////////////////////////////////////////////

ezFloat16Vec4::ezFloat16Vec4(const ezVec4& vec)
{
  operator=(vec);
}

void ezFloat16Vec4::operator=(const ezVec4& vec)
{
  x = vec.x;
  y = vec.y;
  z = vec.z;
  w = vec.w;
}

ezFloat16Vec4::operator ezVec4() const
{
  return ezVec4(x, y, z, w);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Float16);
