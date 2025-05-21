#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgPhysicsAddImpulse;

namespace JPH
{
  class Constraint;
}

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

/// \brief How a rope end gets attached to the anchor point.
struct ezJoltRopeAnchorConstraintMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    None,  ///< The rope is not attached at this anchor and will fall down here once simulation starts.
    Point, ///< The rope end can rotate freely around the anchor point.
    Fixed, ///< The rope end can neither move nor rotate at the anchor point.
    Cone,  ///< The rope end can rotate up to a maximum angle at the anchor point.

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

/// \brief Creates a physically simulated rope that is made up of multiple segments.
///
/// The rope will be created between two anchor points. The component requires at least one anchor to be provided,
/// if no second anchor is given, the rope's owner is used as the second.
///
/// If the anchors themselves are physically simulated bodies, the rope will attach to those bodies,
/// making it possible to constrain physics objects with a rope.
class EZ_JOLTPLUGIN_DLL ezJoltRopeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltRopeComponent, ezComponent, ezJoltRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltRopeComponent

public:
  ezJoltRopeComponent();
  ~ezJoltRopeComponent();

  /// \brief How strongly gravity pulls the rope down.
  void SetGravityFactor(float fGravity);                      // [ property ]
  float GetGravityFactor() const { return m_fGravityFactor; } // [ property ]

  /// \brief The ezSurfaceResource to be used on the rope physics bodies.
  void SetSurfaceFile(ezStringView sFile); // [ property ]
  ezStringView GetSurfaceFile() const;     // [ property ]

  /// Defines which other physics objects the rope collides with.
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// Of how many pieces the rope is made up.
  ezUInt16 m_uiPieces = 16; // [ property ]

  /// How thick the simulated rope is. This is independent of the rope render thickness.
  float m_fThickness = 0.05f; // [ property ]

  /// How much the rope should sag. A value of 0 means it should be absolutely straight.
  float m_fSlack = 0.3f; // [ property ]

  /// If enabled, a more precise simulation method is used, preventing the rope from tunneling through walls.
  /// This comes at an extra performance cost.
  bool m_bCCD = false; // [ property ]

  /// How much each rope segment may bend.
  ezAngle m_MaxBend = ezAngle::MakeFromDegree(30); // [ property ]

  /// How much each rope segment may twist.
  ezAngle m_MaxTwist = ezAngle::MakeFromDegree(15); // [ property ]

  ezUInt8 m_uiWeightCategory = 0;                   // [ property ]
  ezFloat16 m_fWeightScale = 1.0f;                  // [ property ]
  ezFloat16 m_fWeightMass = 5.0f;                   // [ property ]

  /// \brief Sets the anchor 1 references by object GUID.
  void SetAnchor1Reference(const char* szReference); // [ property ]

  /// \brief Sets the anchor 2 references by object GUID.
  void SetAnchor2Reference(const char* szReference); // [ property ]

  /// \brief Sets the anchor 1 reference.
  void SetAnchor1(ezGameObjectHandle hActor);

  /// \brief Sets the anchor 2 reference.
  void SetAnchor2(ezGameObjectHandle hActor);

  /// \brief Adds an impulse (like an impact) to the rope.
  void AddImpulseAtPos(ezMsgPhysicsAddImpulse& ref_msg);

  /// \brief Configures how the rope is attached at anchor 1.
  void SetAnchor1ConstraintMode(ezEnum<ezJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
  ezEnum<ezJoltRopeAnchorConstraintMode> GetAnchor1ConstraintMode() const { return m_Anchor1ConstraintMode; } // [ property ]

  /// \brief Configures how the rope is attached at anchor 2.
  void SetAnchor2ConstraintMode(ezEnum<ezJoltRopeAnchorConstraintMode> mode);                                 // [ property ]
  ezEnum<ezJoltRopeAnchorConstraintMode> GetAnchor2ConstraintMode() const { return m_Anchor2ConstraintMode; } // [ property ]

  /// \brief Makes sure that the rope's connection to a removed body also gets removed.
  void OnJoltMsgDisconnectConstraints(ezJoltMsgDisconnectConstraints& ref_msg); // [ msg handler ]

private:
  void CreateRope();
  ezResult CreateSegmentTransforms(ezDynamicArray<ezTransform>& transforms, float& out_fPieceLength, ezGameObjectHandle hAnchor1, ezGameObjectHandle hAnchor2);
  void DestroyPhysicsShapes();
  void Update();
  void SendPreviewPose();
  const ezJoltMaterial* GetJoltMaterial();
  JPH::Constraint* CreateConstraint(const ezGameObjectHandle& hTarget, const ezTransform& dstLoc, ezUInt32 uiBodyID, ezJoltRopeAnchorConstraintMode::Enum mode, ezUInt32& out_uiConnectedToBodyID);
  void UpdatePreview();

  float GetWeight_Scale() const { return m_fWeightScale; }
  float GetWeight_Mass() const { return m_fWeightMass; }
  void SetWeight_Scale(float fValue) { m_fWeightScale = fValue; }
  void SetWeight_Mass(float fValue) { m_fWeightMass = fValue; }

  ezSurfaceResourceHandle m_hSurface;

  ezGameObjectHandle m_hAnchor1;
  ezGameObjectHandle m_hAnchor2;

  ezEnum<ezJoltRopeAnchorConstraintMode> m_Anchor1ConstraintMode; // [ property ]
  ezEnum<ezJoltRopeAnchorConstraintMode> m_Anchor2ConstraintMode; // [ property ]

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
