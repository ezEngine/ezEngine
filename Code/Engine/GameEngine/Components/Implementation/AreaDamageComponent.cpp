#include <PCH.h>
#include <GameEngine/Components/AreaDamageComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Messages/DamageMessage.h>

EZ_BEGIN_COMPONENT_TYPE(ezAreaDamageComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OnCreation", m_bTriggerOnCreation)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Damage", m_fDamage)->AddAttributes(new ezDefaultValueAttribute(10.0f)),
    EZ_MEMBER_PROPERTY("Impulse", m_fImpulse)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
    //  EZ_BEGIN_MESSAGEHANDLERS
    //{
    //  EZ_MESSAGE_HANDLER(ezInternalComponentMessage, OnTriggered),
    //}
    //EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezSphereVisualizerAttribute("Radius", ezColor::OrangeRed),
    new ezSphereManipulatorAttribute("Radius"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

static ezPhysicsOverlapResult g_OverlapResults;

ezAreaDamageComponent::ezAreaDamageComponent() { }

void ezAreaDamageComponent::ApplyAreaDamage()
{
  if (!IsActiveAndSimulating())
    return;

  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetManager()->m_pPhysicsInterface;

  if (pPhysicsInterface == nullptr)
    return;

  const ezVec3 vOwnPosition = GetOwner()->GetGlobalPosition();

  pPhysicsInterface->QueryDynamicShapesInSphere(m_fRadius, vOwnPosition, m_uiCollisionLayer, g_OverlapResults);

  const float fInvRadius = 1.0f / m_fRadius;

  for (const auto& hit : g_OverlapResults.m_Results)
  {
    if (!hit.m_hActorObject.IsInvalidated())
    {
      ezGameObject* pObject = nullptr;
      if (GetWorld()->TryGetObject(hit.m_hActorObject, pObject))
      {
        const ezVec3 vTargetPos = pObject->GetGlobalPosition();
        const ezVec3 vDistToTarget = vTargetPos - vOwnPosition;
        ezVec3 vDirToTarget = vDistToTarget;
        const float fDistance = vDirToTarget.GetLength();

        if (fDistance >= 0.01f)
        {
          vDirToTarget /= fDistance;
        }
        else
        {
          vDirToTarget.CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
        }

        // linearly scale damage and impulse down by distance
        const float fScale = 1.0f - ezMath::Min(fDistance * fInvRadius, 1.0f);

        // apply a physical impulse
        if (m_fImpulse > 0.0f)
        {
          ezPhysicsAddImpulseMsg msg;
          msg.m_vGlobalPosition = vTargetPos;
          msg.m_vImpulse = vDirToTarget * m_fImpulse * fScale;
          msg.m_uiShapeId = hit.m_uiShapeId;

          pObject->SendMessage(msg);
        }

        // apply damage
        if (m_fDamage > 0.0f)
        {
          ezDamageMessage msg;
          msg.m_fDamage = m_fDamage * fScale;

          pObject->SendMessage(msg);
        }
      }
    }
  }
}


void ezAreaDamageComponent::OnSimulationStarted()
{
  if (m_bTriggerOnCreation)
  {
    ApplyAreaDamage();
  }
}

void ezAreaDamageComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_bTriggerOnCreation;
  s << m_fRadius;
  s << m_uiCollisionLayer;
  s << m_fDamage;
  s << m_fImpulse;
}

void ezAreaDamageComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_bTriggerOnCreation;
  s >> m_fRadius;
  s >> m_uiCollisionLayer;
  s >> m_fDamage;
  s >> m_fImpulse;
}

//////////////////////////////////////////////////////////////////////////

ezAreaDamageComponentManager::ezAreaDamageComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
  , m_pPhysicsInterface(nullptr)
{
}

void ezAreaDamageComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAreaDamageComponentManager::Update, this);

  RegisterUpdateFunction(desc);

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
}

void ezAreaDamageComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  // nothing to do atm
}

