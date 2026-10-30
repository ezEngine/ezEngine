#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleAreaComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezParticleAreaComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SpawnScale", m_fSpawnScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezParticleAreaComponent::ezParticleAreaComponent() = default;
ezParticleAreaComponent::~ezParticleAreaComponent() = default;

void ezParticleAreaComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fSpawnScale;
}

void ezParticleAreaComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fSpawnScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleBoxAreaComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new ezDefaultValueAttribute(ezVec3(2.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("Size", 1.0f, true),
    new ezBoxVisualizerAttribute("Size", 1.0f, ezColor::MediumVioletRed),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleBoxAreaComponent::ezParticleBoxAreaComponent() = default;
ezParticleBoxAreaComponent::~ezParticleBoxAreaComponent() = default;

void ezParticleBoxAreaComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
}

void ezParticleBoxAreaComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleSphereAreaComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColor::MediumVioletRed),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleSphereAreaComponent::ezParticleSphereAreaComponent() = default;
ezParticleSphereAreaComponent::~ezParticleSphereAreaComponent() = default;

void ezParticleSphereAreaComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void ezParticleSphereAreaComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezParticleCylinderAreaComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveZ, "Height", "Radius", ezColor::MediumVioletRed),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezParticleCylinderAreaComponent::ezParticleCylinderAreaComponent() = default;
ezParticleCylinderAreaComponent::~ezParticleCylinderAreaComponent() = default;

void ezParticleCylinderAreaComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
}

void ezParticleCylinderAreaComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
}

//////////////////////////////////////////////////////////////////////////

void ezParticleAreaReference::Reset()
{
  m_bSearched = false;
  m_hArea.Invalidate();
  m_Transform.SetIdentity();
  m_fSpawnCountMultiplier = 1.0f;
  m_bAreaUsed = false;
}

bool ezParticleAreaReference::IsReady(const ezParticleEffectInstance& effect) const
{
  return m_bSearched || effect.GetOwnerObject().IsInvalidated();
}

const ezParticleAreaComponent* ezParticleAreaReference::Update(const ezWorld& world, const ezParticleEffectInstance& effect, const ezRTTI* pAreaType)
{
  if (!m_bSearched)
  {
    m_bSearched = true;

    const ezGameObject* pOwner = nullptr;
    if (world.TryGetObject(effect.GetOwnerObject(), pOwner))
    {
      // areas of other types are meant for other initializers, e.g. a box and a sphere area on the same object
      for (const ezGameObject* pObject = pOwner; pObject != nullptr; pObject = pObject->GetParent())
      {
        const ezComponent* pArea = nullptr;
        if (pObject->TryGetComponentOfBaseType(pAreaType, pArea))
        {
          m_hArea = pArea->GetHandle();
          break;
        }
      }
    }
  }

  if (m_hArea.IsInvalidated())
    return nullptr;

  const ezParticleAreaComponent* pArea = nullptr;
  if (!world.TryGetComponent(m_hArea, pArea) || !pArea->IsActiveAndInitialized())
    return nullptr;

  m_Transform = ezTransform::MakeLocalTransform(effect.GetTransformForNextFrame(), pArea->GetOwner()->GetGlobalTransform());
  return pArea;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleAreaComponent);
