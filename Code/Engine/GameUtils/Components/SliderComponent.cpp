#include <GameUtils/PCH.h>
#include <GameUtils/Components/SliderComponent.h>

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, float fTimeSinceStartInSec);

EZ_BEGIN_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, 1, ezSliderComponentManager);
  //EZ_BEGIN_PROPERTIES
  //  EZ_ENUM_MEMBER_PROPERTY("Axis", ezRotorComponentAxis, m_Axis),
  //  EZ_MEMBER_PROPERTY("Degrees to Rotate", m_iDegreeToRotate),
  //  EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
  //  EZ_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  //EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSliderComponent::ezSliderComponent()
{
}

void ezSliderComponent::Update()
{
}

