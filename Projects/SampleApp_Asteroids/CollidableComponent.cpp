#include "Level.h"
#include "CollidableComponent.h"

EZ_IMPLEMENT_COMPONENT_TYPE(CollidableComponent, CollidableComponentManager);

CollidableComponent::CollidableComponent()
{
  m_fCollisionRadius = 1.0f;
}
