#include <PCH.h>
#include <RTS/Components/ObstacleComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ObstacleComponent, ezComponent, 1, ObstacleComponentManager);
EZ_END_COMPONENT_TYPE();

float ObstacleComponent::g_fDefaultRadius = 1.5f;

ObstacleComponent::ObstacleComponent()
{
  m_fRadius = 0.0f;
}
