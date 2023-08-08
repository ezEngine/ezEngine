#include "CollidableComponent.h"
#include "Level.h"

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(CollidableComponent, 1, ezComponentMode::Static)
EZ_END_COMPONENT_TYPE
// clang-format on

CollidableComponent::CollidableComponent()
{
  m_fCollisionRadius = 1.0f;
}
