#include <PCH.h>
#include <RTS/Components/ObstacleComponent.h>

EZ_IMPLEMENT_COMPONENT_TYPE(ObstacleComponent, ObstacleComponentManager);

float ObstacleComponent::g_fDefaultRadius = 1.5f;

ObstacleComponent::ObstacleComponent()
{
  m_fRadius = 0.0f;
}
