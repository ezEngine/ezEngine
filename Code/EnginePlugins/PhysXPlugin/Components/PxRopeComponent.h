#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;
namespace physx
{
  class PxSphericalJoint;
}

namespace physx
{
  class PxArticulationLink;
  class PxArticulation;
  class PxAggregate;
} // namespace physx

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

using ezPxRopeComponentManager = ezComponentManagerSimple<class ezPxRopeComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact>;

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxRopeComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxRopeComponent, ezPxComponent, ezPxRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxRopeComponent

  bool GetDisableGravity() const { return m_bDisableGravity; } // [ property ]
  void SetDisableGravity(bool b);                              // [ property ]

public:
  ezPxRopeComponent();
  ~ezPxRopeComponent();

  void SetLength(float val);                    // [ property ]
  float GetLength() const { return m_fLength; } // [ property ]
  void SetSlack(float val);                     // [ property ]
  float GetSlack() const { return m_fSlack; }   // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]
  ezUInt16 m_uiPieces = 16;       // [ property ]
  float m_fThickness = 0.05f;     // [ property ]
  bool m_bFixAtStart = true;      // [ property ]
  bool m_bFixAtEnd = true;        // [ property ]

  void AddForceAtPos(ezMsgPhysicsAddForce& msg);
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg);

private:
  void CreateRope();
  void CreateSegmentTransforms(const ezTransform& rootTransform, ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const;
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  PxFilterData CreateFilterData();
  PxMaterial* GetPxMaterial();

  ezSurfaceResourceHandle m_hSurface;

  float m_fLength = 2.0f;
  float m_fSlack = 0.1f;
  ezUInt32 m_uiShapeID = ezInvalidIndex;
  bool m_bSelfCollision = true;
  bool m_bDisableGravity = false;

  physx::PxAggregate* m_pAggregate = nullptr;
  physx::PxArticulation* m_pArticulation = nullptr;
  physx::PxSphericalJoint* m_pJointStart = nullptr;
  physx::PxSphericalJoint* m_pJointEnd = nullptr;
  ezDynamicArray<physx::PxArticulationLink*> m_ArticulationLinks;

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
