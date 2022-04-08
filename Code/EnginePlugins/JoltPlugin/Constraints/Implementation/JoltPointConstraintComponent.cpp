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

void ezJoltPointConstraintComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  // auto& s = stream.GetStream();
}

void ezJoltPointConstraintComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}

void ezJoltPointConstraintComponent::ApplySettings()
{
  SUPER::ApplySettings();
}

void ezJoltPointConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  JPH::PointConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
  const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = ezJoltConversionUtils::ToVec3(m_localFrameA.m_vPosition) + diff0;
  opt.mPoint2 = ezJoltConversionUtils::ToVec3(m_localFrameB.m_vPosition) + diff1;

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}
