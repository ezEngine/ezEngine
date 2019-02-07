#include <RecastPluginPCH.h>

#include <Foundation/Math/Vec3.h>
#include <RecastPlugin/Utils/RcMath.h>

ezRcPos::ezRcPos()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_Pos[0] = ezMath::BasicType<float>::GetNaN();
  m_Pos[1] = ezMath::BasicType<float>::GetNaN();
  m_Pos[2] = ezMath::BasicType<float>::GetNaN();
#endif
}

ezRcPos::ezRcPos(const ezVec3& v)
{
  *this = v;
}

ezRcPos::ezRcPos(const float* pos)
{
  *this = pos;
}

ezRcPos::operator const float*() const
{
  return &m_Pos[0];
}

ezRcPos::operator float*()
{
  return &m_Pos[0];
}

ezRcPos::operator ezVec3() const
{
  return ezVec3(m_Pos[0], m_Pos[2], m_Pos[1]);
}

void ezRcPos::operator=(const float* pos)
{
  m_Pos[0] = pos[0];
  m_Pos[1] = pos[1];
  m_Pos[2] = pos[2];
}

void ezRcPos::operator=(const ezVec3& v)
{
  m_Pos[0] = v.x;
  m_Pos[1] = v.z;
  m_Pos[2] = v.y;
}
