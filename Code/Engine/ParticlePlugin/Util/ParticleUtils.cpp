#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Util/ParticleUtils.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVarianceType, ezNoBase, 1, ezRTTIDefaultAllocator<ezVarianceType>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_fValue)->AddAttributes(new ezDefaultValueAttribute(23.45f)),
    EZ_MEMBER_PROPERTY("Variance", m_fVariance)
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

ezVarianceType::ezVarianceType()
{
  m_fValue = 0.0f;
  m_fVariance = 0.0f;
}
