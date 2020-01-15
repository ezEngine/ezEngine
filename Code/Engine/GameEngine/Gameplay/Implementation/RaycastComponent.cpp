
#include <GameEnginePCH.h>

#include <GameEngine/Gameplay/RaycastComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/Messages/TriggerMessage.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRaycastComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::YellowGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRaycastComponent::ezRaycastComponent() = default;

ezRaycastComponent::~ezRaycastComponent() = default;


void ezRaycastComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  stream.WriteGameObjectHandle(m_hRaycastEndObject);
  s << m_fMaxDistance;
}

void ezRaycastComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  auto& s = stream.GetStream();

  m_hRaycastEndObject = stream.ReadGameObjectHandle();
  s >> m_fMaxDistance;
}

void ezRaycastComponent::SetRaycastEndObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hRaycastEndObject = resolver(szReference, GetHandle(), "RaycastEndObject");
}

void ezRaycastComponent::Update()
{
  if (m_hRaycastEndObject.IsInvalidated())
    return;

  ezGameObject* pEndObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    return;
  }

  auto* pPhysicsModule = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  ezPhysicsHitResult hit;
  if (pPhysicsModule->CastRay(GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalDirForwards(), m_fMaxDistance, 0, hit))
  {
    pEndObject->SetGlobalPosition(GetOwner()->GetGlobalPosition() + hit.m_fDistance * GetOwner()->GetGlobalDirForwards());
    // TODO: Set orientation?
  }
  else
  {
    // TODO: Decide what happens when the ray doesn't hit
  }

}
