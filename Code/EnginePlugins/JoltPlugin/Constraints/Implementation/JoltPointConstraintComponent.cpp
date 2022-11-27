#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <JoltPlugin/Constraints/JoltPointConstraintComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltPointConstraintComponent, 1, ezComponentMode::Static)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltPointConstraintComponent::ezJoltPointConstraintComponent() = default;
ezJoltPointConstraintComponent::~ezJoltPointConstraintComponent() = default;

void ezJoltPointConstraintComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  // auto& s = stream.GetStream();
}

void ezJoltPointConstraintComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}

void ezJoltPointConstraintComponent::ApplySettings()
{
  SUPER::ApplySettings();
}

void ezJoltPointConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::PointConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * ezJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * ezJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
