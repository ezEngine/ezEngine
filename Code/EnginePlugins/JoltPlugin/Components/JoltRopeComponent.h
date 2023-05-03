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

struct ezJoltRopeAnchorConstraintMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    None,
    Point,
    Fixed,
    Cone,

    Default = Point
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltRopeAnchorConstraintMode);

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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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

  ezUInt8 m_uiCollisionLayer = 0;                                 // [ property ]
  ezUInt16 m_uiPieces = 16;                                       // [ property ]
  float m_fThickness = 0.05f;                                     // [ property ]
  float m_fSlack = 0.3f;                                          // [ property ]
  ezEnum<ezJoltRopeAnchorConstraintMode> m_Anchor1ConstraintMode; // [ property ]
  ezEnum<ezJoltRopeAnchorConstraintMode> m_Anchor2ConstraintMode; // [ property ]
  bool m_bCCD = false;                                            // [ property ]
  ezAngle m_MaxBend = ezAngle::Degree(30);                        // [ property ]
  ezAngle m_MaxTwist = ezAngle::Degree(15);                       // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]

  void SetAnchor1(ezGameObjectHandle hActor);
  void SetAnchor2(ezGameObjectHandle hActor);

  void AddForceAtPos(ezMsgPhysicsAddForce& ref_msg);
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);

  /// \brief Makes sure that the rope's connection to a removed body also gets removed.
  void OnJoltMsgDisconnectConstraints(ezJoltMsgDisconnectConstraints& msg); // [ msg handler ]

private:
  void CreateRope();
  ezResult CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength, ezGameObjectHandle hAnchor1, ezGameObjectHandle hAnchor2);
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  const ezJoltMaterial* GetJoltMaterial();
  JPH::Constraint* CreateConstraint(const ezGameObjectHandle& hTarget, const ezTransform& dstLoc, ezUInt32 uiBodyID, ezJoltRopeAnchorConstraintMode::Enum mode, ezUInt32& out_uiConnectedToBodyID);
  void UpdatePreview();

  ezSurfaceResourceHandle m_hSurface;

  ezGameObjectHandle m_hAnchor1;
  ezGameObjectHandle m_hAnchor2;

  float m_fTotalMass = 1.0f;
  float m_fMaxForcePerFrame = 0.0f;
  float m_fBendStiffness = 0.0f;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  bool m_bSelfCollision = false;
  float m_fGravityFactor = 1.0f;
  ezUInt32 m_uiPreviewHash = 0;

  JPH::Ragdoll* m_pRagdoll = nullptr;
  JPH::Constraint* m_pConstraintAnchor1 = nullptr;
  JPH::Constraint* m_pConstraintAnchor2 = nullptr;
  ezUInt32 m_uiAnchor1BodyID = ezInvalidIndex;
  ezUInt32 m_uiAnchor2BodyID = ezInvalidIndex;


private:
  const char* DummyGetter() const { return nullptr; }
};
