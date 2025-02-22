#pragma once

#include <Core/Interfaces/PhysicsQuery.h>
#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>

struct ezGameObjectHandle;
struct ezSkeletonResourceDescriptor;

class EZ_CORE_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

protected:
  ezPhysicsWorldModuleInterface(ezWorld* pWorld)
    : ezWorldModule(pWorld)
  {
  }

public:
  /// \brief Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns ezInvalidIndex if no such collision layer exists.
  virtual ezUInt32 GetCollisionLayerByName(ezStringView sName) const = 0;

  virtual bool Raycast(ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(ezPhysicsOverlapResultArray& out_results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const = 0;

  virtual ezVec3 GetGravity() const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// \brief Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(ezGameObject* pOwner, ezVec3 vBoxSize)
  {
    EZ_IGNORE_UNUSED(pOwner);
    EZ_IGNORE_UNUSED(vBoxSize);
  }

  struct JointConfig
  {
    ezGameObjectHandle m_hActorA;
    ezGameObjectHandle m_hActorB;
    ezTransform m_LocalFrameA = ezTransform::MakeIdentity();
    ezTransform m_LocalFrameB = ezTransform::MakeIdentity();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// \brief Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(ezGameObject* pOwner, const ezPhysicsWorldModuleInterface::FixedJointConfig& cfg)
  {
    EZ_IGNORE_UNUSED(pOwner);
    EZ_IGNORE_UNUSED(cfg);
  }

  /// \brief Gets world space bounds of a physics object if its shape type is included in shapeTypes and its collision layer interacts with uiCollisionLayer.
  virtual ezBoundingBoxSphere GetWorldSpaceBounds(ezGameObject* pOwner, ezUInt32 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
  {
    EZ_IGNORE_UNUSED(pOwner);
    EZ_IGNORE_UNUSED(uiCollisionLayer);
    EZ_IGNORE_UNUSED(shapeTypes);
    EZ_IGNORE_UNUSED(bIncludeChildObjects);
    return ezBoundingBoxSphere::MakeInvalid();
  }
};

/// \brief Used to apply a physical impulse on the object
struct EZ_CORE_DLL ezMsgPhysicsAddImpulse : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddImpulse, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vImpulse;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

/// \brief Used to apply a physical force on the object
struct EZ_CORE_DLL ezMsgPhysicsAddForce : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddForce, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vForce;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct EZ_CORE_DLL ezMsgPhysicsJointBroke : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsJointBroke, ezEventMessage);

  ezGameObjectHandle m_hJointObject;
};

/// \brief Sent by components such as ezJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct EZ_CORE_DLL ezMsgObjectGrabbed : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgObjectGrabbed, ezMessage);

  ezGameObjectHandle m_hGrabbedBy;
  bool m_bGotGrabbed = true;
};

/// \brief Send this to components such as ezJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct EZ_CORE_DLL ezMsgReleaseObjectGrab : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgReleaseObjectGrab, ezMessage);

  ezGameObjectHandle m_hGrabbedObjectToRelease;
};

//////////////////////////////////////////////////////////////////////////

struct EZ_CORE_DLL ezSmcTriangle
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiVertexIndices[3];
};

struct EZ_CORE_DLL ezSmcSubMesh
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiFirstTriangle = 0;
  ezUInt32 m_uiNumTriangles = 0;
  ezUInt16 m_uiSurfaceIndex = 0;
};

struct EZ_CORE_DLL ezSmcDescription
{
  ezDeque<ezVec3> m_Vertices;
  ezDeque<ezSmcTriangle> m_Triangles;
  ezDeque<ezSmcSubMesh> m_SubMeshes;
  ezDeque<ezString> m_Surfaces;
};

struct EZ_CORE_DLL ezMsgBuildStaticMesh : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgBuildStaticMesh, ezMessage);

  /// \brief Append data to this description to add meshes to the automatic static mesh generation
  ezSmcDescription* m_pStaticMeshDescription = nullptr;
};
