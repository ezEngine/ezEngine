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

  ezVec3 m_vMainCameraPosition = ezVec3::MakeZero();
  ezHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  ezHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;
  ezDeque<PxConstraint*> m_BrokenConstraints;
  ezWorld* m_pWorld = nullptr;

  SlideAndRollInfo* FindSlideOrRollInfo(PxRigidDynamic* pActor, const ezVec3& vAvgPos);

  virtual void onConstraintBreak(PxConstraintInfo* pConstraints, PxU32 count) override;

  virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pPairs, PxU32 pairs) override;

  void OnContact_SlideReaction(const ezVec3& vAvgPos, const ezVec3& vAvgNormal, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ref_contactFlags0, ezBitflags<ezOnPhysXContact>& ref_contactFlags1);
  void OnContact_RollReaction(const ezVec3& vAvgPos, PxRigidDynamic* pRigid0, PxRigidDynamic* pRigid1, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ref_contactFlags0, ezBitflags<ezOnPhysXContact>& ref_contactFlags1);
  void OnContact_SlideAndRollReaction(const PxContactPairHeader& pairHeader, const PxContactPair& pair, ezBitflags<ezOnPhysXContact>& ref_contactFlags0, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, ezBitflags<ezOnPhysXContact>& ref_contactFlags1, const ezUInt32 uiNumContactPoints, ezBitflags<ezOnPhysXContact>& ref_combinedContactFlags);

  void OnContact_ImpactReaction(PxContactPairPoint* pContactPointBuffer, const PxContactPair& pair, const ezVec3& vAvgPos, const ezVec3& vAvgNormal, float fMaxImpactSqr, const PxContactPairHeader& pairHeader);

  void SendContactReport(const PxContactPairHeader& pairHeader, const PxContactPair* pPairs, PxU32 pairs);

  struct TriggerEvent
  {
    ezComponentHandle m_hTriggerComponent;
    ezComponentHandle m_hOtherComponent;
    ezTriggerState::Enum m_TriggerState;
  };

  ezDeque<TriggerEvent> m_TriggerEvents;

  virtual void onTrigger(PxTriggerPair* pPairs, PxU32 count) override;

  virtual void onWake(PxActor** pActors, PxU32 count) override {}

  virtual void onSleep(PxActor** pActors, PxU32 count) override {}

  virtual void onAdvance(const PxRigidBody* const* pBodyBuffer, const PxTransform* pPoseBuffer, const PxU32 count) override {}
};
