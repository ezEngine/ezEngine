#pragma once

#include <PhysXPlugin/PhysXInterface.h>

#include <PxPhysicsAPI.h>
using namespace physx;

class ezPxSimulationEventCallback : public PxSimulationEventCallback
{
public:
  struct InteractionContact
  {
    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezSurfaceResource* m_pSurface;
    ezTempHashedString m_sInteraction;
    float m_fImpulseSqr;
    float m_fDistanceSqr;
  };

  struct SlideAndRollInfo
  {
    PxRigidDynamic* m_pActor = nullptr;
    bool m_bStillSliding = false;
    bool m_bStillRolling = false;

    float m_fDistanceSqr;
    ezVec3 m_vContactPosition;
    ezGameObjectHandle m_hSlidePrefab;
    ezGameObjectHandle m_hRollPrefab;
    ezHashedString m_sSlideInteractionPrefab;
    ezHashedString m_sRollInteractionPrefab;
  };

  ezVec3 m_vMainCameraPosition = ezVec3::ZeroVector();
  ezHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  ezHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;
  ezDeque<PxConstraint*> m_BrokenConstraints;
  ezWorld* m_pWorld = nullptr;

  SlideAndRollInfo* FindSlideOrRollInfo(PxRigidDynamic* pActor, const ezVec3& vAvgPos);

  virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;

  virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

  void OnContact_SlideReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ContactFlags0, ezBitflags<ezOnPhysXContact>& ContactFlags1);
  void OnContact_RollReaction(const ezVec3& vAvgPos, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ContactFlags0, ezBitflags<ezOnPhysXContact>& ContactFlags1);
  void OnContact_SlideAndRollReaction(const PxContactPairHeader& pairHeader, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ContactFlags0, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnPhysXContact>& ContactFlags1, const ezUInt32 uiNumContactPoints, ezBitflags<ezOnPhysXContact>& CombinedContactFlags);

  void OnContact_ImpactReaction(PxContactPairPoint* contactPointBuffer, const PxContactPair& pair, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const PxContactPairHeader& pairHeader);

  void SendContactReport(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);

  struct TriggerEvent
  {
    ezComponentHandle m_hTriggerComponent;
    ezComponentHandle m_hOtherComponent;
    ezTriggerState::Enum m_TriggerState;
  };

  ezDeque<TriggerEvent> m_TriggerEvents;

  virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

  virtual void onWake(PxActor** actors, PxU32 count) override {}

  virtual void onSleep(PxActor** actors, PxU32 count) override {}

  virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
};
