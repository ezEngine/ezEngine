#pragma once

#include <Core/World/Declarations.h>
#include <Physics/Collision/ContactListener.h>
#include <Physics/SoftBody/SoftBodyContactListener.h>

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

  ezMutex m_Mutex;
  ezWorld* m_pWorld = nullptr;
  ezVec3 m_vMainCameraPosition = ezVec3::MakeZero();
  ezHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  ezHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;

  SlideAndRollInfo* FindSlideOrRollInfo(const JPH::Body* pBody, const ezVec3& vAvgPos);

  void OnContact_SlideReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, ezBitflags<ezOnJoltContact> onContact0, ezBitflags<ezOnJoltContact> onContact1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal);

  void OnContact_RollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, ezBitflags<ezOnJoltContact> onContact0, ezBitflags<ezOnJoltContact> onContact1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal0);

  void OnContact_ImpactReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const ezSurfaceResource* pSurface1, const ezSurfaceResource* pSurface2, bool bActor1StaticOrKinematic);
  void OnContact_SlideAndRollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, ezBitflags<ezOnJoltContact> onContact0, ezBitflags<ezOnJoltContact> onContact1, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnJoltContact> combinedContactFlags);

  void SpawnPhysicsImpactReactions();
  void UpdatePhysicsSlideReactions();
  void UpdatePhysicsRollReactions();
};

class ezJoltContactListener : public JPH::ContactListener
{
public:
  ezWorld* m_pWorld = nullptr;
  ezJoltContactEvents m_ContactEvents;

  struct TriggerObj
  {
    const ezJoltTriggerComponent* m_pTrigger = nullptr;
    ezGameObjectHandle m_hTarget;
  };

  ezMutex m_TriggerMutex;
  ezMap<ezUInt64, TriggerObj> m_Trigs;

  void RemoveTrigger(const ezJoltTriggerComponent* pTrigger);

  virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;
  virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;

  virtual void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;

  void OnContact(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings, bool bPersistent, bool bIsDebrisContact);

  bool ActivateTrigger(const JPH::Body& body1, const JPH::Body& body2, ezUInt64 uiBody1id, ezUInt64 uiBody2id);

  void DeactivateTrigger(ezUInt64 uiBody1id, ezUInt64 uiBody2id);
};

class ezJoltSoftBodyContactListener : public JPH::SoftBodyContactListener
{
public:
  virtual JPH::SoftBodyValidateResult OnSoftBodyContactValidate(const JPH::Body& softBody, const JPH::Body& otherBody, JPH::SoftBodyContactSettings& ref_settings) override;
};
