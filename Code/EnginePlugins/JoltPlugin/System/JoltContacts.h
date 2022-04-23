#pragma once

#include <Core/World/Declarations.h>
#include <Physics/Collision/ContactListener.h>

class ezWorld;
class ezJoltTriggerComponent;
class ezJoltContactEvents;

namespace JPH
{
  class SubShapeIDPair;
  class ContactSettings;
  class ContactManifold;
  class Body;
} // namespace JPH

class ezJoltContactEvents
{
public:
  struct InteractionContact
  {
    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    const ezSurfaceResource* m_pSurface;
    ezTempHashedString m_sInteraction;
    float m_fImpulseSqr;
    float m_fDistanceSqr;
  };

  struct SlideAndRollInfo
  {
    const JPH::Body* m_pBody = nullptr;
    bool m_bStillSliding = false;
    bool m_bStillRolling = false;

    float m_fDistanceSqr;
    ezVec3 m_vContactPosition;
    ezGameObjectHandle m_hSlidePrefab;
    ezGameObjectHandle m_hRollPrefab;
    ezHashedString m_sSlideInteractionPrefab;
    ezHashedString m_sRollInteractionPrefab;
  };

  ezWorld* m_pWorld = nullptr;
  ezVec3 m_vMainCameraPosition = ezVec3::ZeroVector();
  ezHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  ezHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;

  SlideAndRollInfo* FindSlideOrRollInfo(const JPH::Body* pBody, const ezVec3& vAvgPos);

  void OnContact_SlideReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, const ezJoltActorComponent* pActor0, const ezJoltActorComponent* pActor1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal);

  void OnContact_RollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, const ezJoltActorComponent* pActor0, const ezJoltActorComponent* pActor1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal0);

  void OnContact_ImpactReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const ezSurfaceResource* pSurface1, const ezSurfaceResource* pSurface2, bool bActor1StaticOrKinematic);
  void OnContact_SlideAndRollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, const ezJoltActorComponent* pActor0, const ezJoltActorComponent* pActor1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnJoltContact> CombinedContactFlags);

  void SpawnPhysicsImpactReactions();
  void UpdatePhysicsSlideReactions();
  void UpdatePhysicsRollReactions();
};

class ezJoltContactListener : public JPH::ContactListener
{
public:
  ezWorld* m_pWorld = nullptr;
  ezJoltContactEvents m_Events;

  struct TriggerObj
  {
    const ezJoltTriggerComponent* m_pTrigger = nullptr;
    ezGameObjectHandle m_hTarget;
  };

  ezMap<ezUInt64, TriggerObj> m_Trigs;

  virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
  virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

  virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

  void OnContact(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings, bool bPersistent);

  bool ActivateTrigger(const JPH::Body& inBody1, const JPH::Body& inBody2, ezUInt64 uiBody1id, ezUInt64 uiBody2id);

  void DeactivateTrigger(ezUInt64 uiBody1id, ezUInt64 uiBody2id);
};
