#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <JoltPlugin/Constraints/JoltFixedConstraintComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltFixedConstraintComponent, 1, ezComponentMode::Static);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltFixedConstraintComponent::ezJoltFixedConstraintComponent() = default;
ezJoltFixedConstraintComponent::~ezJoltFixedConstraintComponent() = default;

void ezJoltFixedConstraintComponent::ApplySettings()
{
  ezJoltConstraintComponent::ApplySettings();
}

void ezJoltFixedConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  JPH::FixedConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
