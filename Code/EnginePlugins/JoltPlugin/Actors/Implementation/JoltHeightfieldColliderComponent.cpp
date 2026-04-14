#include <JoltPlugin/JoltPluginPCH.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltHeightfieldColliderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Actors"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezJoltHeightfieldColliderComponent::ezJoltHeightfieldColliderComponent() = default;
ezJoltHeightfieldColliderComponent::~ezJoltHeightfieldColliderComponent() = default;

void ezJoltHeightfieldColliderComponent::OnDeactivated()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (pModule != nullptr)
  {
    JPH::BodyID bodyId(m_uiJoltBodyID);

    if (!bodyId.IsInvalid())
    {
      auto* pSystem = pModule->GetJoltSystem();
      auto* pBodies = &pSystem->GetBodyInterface();

      if (pBodies->IsAdded(bodyId))
      {
        pBodies->RemoveBody(bodyId);
      }
      else
      {
        pModule->RemoveBodyFromQueue(bodyId);
      }

      pBodies->DestroyBody(bodyId);
      m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    pModule->ReleaseHeightfieldCacheEntry(m_sCacheIdentifier);
  }

  SUPER::OnDeactivated();
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltHeightfieldColliderComponent);
