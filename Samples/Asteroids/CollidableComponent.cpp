#include "Level.h"
#include "CollidableComponent.h"

EZ_BEGIN_COMPONENT_TYPE(CollidableComponent, ezComponent, CollidableComponentManager);
EZ_END_COMPONENT_TYPE();

CollidableComponent::CollidableComponent()
{
  m_fCollisionRadius = 1.0f;
}
