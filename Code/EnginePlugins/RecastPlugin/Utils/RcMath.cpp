#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Math/Vec3.h>
#include <RecastPlugin/Utils/RcMath.h>

ezRcPos::ezRcPos()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_Pos[0] = ezMath::NaN<float>();
  m_Pos[1] = ezMath::NaN<float>();
  m_Pos[2] = ezMath::NaN<float>();
#endif
}

ezRcPos::ezRcPos(const ezVec3& v)
{
  *this = v;
}

ezRcPos::ezRcPos(const float* pPos)
{
  *this = pPos;
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

void ezRcPos::operator=(const float* pPos)
{
  m_Pos[0] = pPos[0];
  m_Pos[1] = pPos[1];
  m_Pos[2] = pPos[2];
}

void ezRcPos::operator=(const ezVec3& v)
{
  m_Pos[0] = v.x;
  m_Pos[1] = v.z;
  m_Pos[2] = v.y;
}
