#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgPhysicsAddImpulse;
struct ezMsgPhysicsAddForce;
namespace JPH
{
  class Constraint;
}

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltRopeComponentManager : public ezComponentManager<class ezJoltRopeComponent, ezBlockStorageType::Compact>
{
public:
  ezJoltRopeComponentManager(ezWorld* pWorld);
  ~ezJoltRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltRopeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltRopeComponent, ezComponent, ezJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltRopeComponent

public:
  ezJoltRopeComponent();
  ~ezJoltRopeComponent();

  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]
  void SetGravityFactor(float fGravity);                      // [ property ]

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  ezUInt8 m_uiCollisionLayer = 0;           // [ property ]
  ezUInt16 m_uiPieces = 16;                 // [ property ]
  float m_fThickness = 0.05f;               // [ property ]
  float m_fSlack = 0.3f;                    // [ property ]
  bool m_bAttachToOrigin = true;            // [ property ]
  bool m_bAttachToAnchor = true;            // [ property ]
  ezAngle m_MaxBend = ezAngle::Degree(30);  // [ property ]
  ezAngle m_MaxTwist = ezAngle::Degree(15); // [ property ]

  void SetAnchorReference(const char* szReference); // [ property ]
  void SetAnchor(ezGameObjectHandle hActor);

  void AddForceAtPos(ezMsgPhysicsAddForce& msg);
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& msg);

private:
  void CreateRope();
  ezResult CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength) const;
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  const ezJoltMaterial* GetJoltMaterial();
  JPH::Constraint* CreateConstraint(const ezGameObjectHandle& hTarget, const ezTransform& location, ezUInt32 uiBodyID);
  void UpdatePreview();

  ezSurfaceResourceHandle m_hSurface;

  ezGameObjectHandle m_hAnchor;

  float m_fTotalMass = 1.0f;
  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  bool m_bSelfCollision = false;
  float m_fGravityFactor = 1.0f;
  ezVec3 m_vPreviewRefPos = ezVec3::ZeroVector();

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::Constraint* m_pConstraintOrigin = nullptr;
  JPH::Constraint* m_pConstraintAnchor = nullptr;


private:
  const char* DummyGetter() const { return nullptr; }
};
