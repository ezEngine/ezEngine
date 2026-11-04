#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/Components/AsteroidComponent.h>
#include <AsteroidsPlugin/Components/AsteroidsGameComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Random.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(AsteroidComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("ExplosionPrefab", m_hExplosionPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Asteroids"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

AsteroidComponent::AsteroidComponent() = default;
AsteroidComponent::~AsteroidComponent() = default;

void AsteroidComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hExplosionPrefab;
}

void AsteroidComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_hExplosionPrefab;
}

void AsteroidComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_hExplosionPrefab.IsValid())
  {
    // make sure the first explosion doesn't cause a hiccup
    ezResourceManager::PreloadResource(m_hExplosionPrefab);
  }
}

void AsteroidComponent::Launch(ezUInt8 uiSize, float fRadius, const ezVec3& vVelocity)
{
  m_uiSize = uiSize;
  m_fRadius = fRadius;
  m_vVelocity = vVelocity;

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();
  m_vSpinAxis = ezVec3::MakeRandomDirection(rng);
  m_SpinSpeed = ezAngle::MakeFromDegree((float)rng.DoubleMinMax(20.0, 90.0));

  GetOwner()->SetLocalUniformScaling(fRadius);
  GetOwner()->SetLocalRotation(ezQuat::MakeFromAxisAndAngle(m_vSpinAxis, ezAngle::MakeFromDegree((float)rng.DoubleMinMax(0.0, 360.0))));
}

void AsteroidComponent::Update()
{
  const float fTimeDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  ezGameObject* pOwner = GetOwner();

  pOwner->SetLocalRotation(ezQuat::MakeFromAxisAndAngle(m_vSpinAxis, m_SpinSpeed * fTimeDiff) * pOwner->GetLocalRotation());

  AsteroidsGameComponent::MoveAndWrap(pOwner, m_vVelocity, m_fRadius);
}
