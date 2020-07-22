#include "CollidableComponent.h"
#include "Level.h"

EZ_BEGIN_COMPONENT_TYPE(CollidableComponent, 1, ezComponentMode::Static)
  ;
EZ_END_COMPONENT_TYPE

CollidableComponent::CollidableComponent()
{
  m_fCollisionRadius = 1.0f;
}
