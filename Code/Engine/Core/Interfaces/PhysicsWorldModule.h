#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>
#include <GameEngine/GameEngineDLL.h>

struct ezGameObjectHandle;
struct ezSkeletonResourceDescriptor;

typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

/// \brief Used for raycast and seep tests
struct ezPhysicsCastResult
{
  ezVec3 m_vPosition;
  ezVec3 m_vNormal;
  float m_fDistance;

  ezGameObjectHandle m_hShapeObject;     ///< The game object to which the hit physics shape is attached.
  ezGameObjectHandle m_hActorObject;     ///< The game object to which the parent actor of the hit physics shape is attached.
  ezSurfaceResourceHandle m_hSurface;    ///< The type of surface that was hit (if available)
  ezUInt32 m_uiShapeId = ezInvalidIndex; ///< The shape id of the hit physics shape
};

struct ezPhysicsCastResultArray
{
  ezHybridArray<ezPhysicsCastResult, 16> m_Results;
};

/// \brief Used to report overlap query results
struct ezPhysicsOverlapResult
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectHandle m_hShapeObject;     ///< The game object to which the hit physics shape is attached.
  ezGameObjectHandle m_hActorObject;     ///< The game object to which the parent actor of the hit physics shape is attached.
  ezUInt32 m_uiShapeId = ezInvalidIndex; ///< The shape id of the hit physics shape
};

struct ezPhysicsOverlapResultArray
{
  ezHybridArray<ezPhysicsOverlapResult, 16> m_Results;
};

EZ_DECLARE_FLAGS(ezUInt32, ezPhysicsShapeType, Static, Dynamic);

struct ezPhysicsQueryParameters
{
  ezPhysicsQueryParameters() = default;
  explicit ezPhysicsQueryParameters(ezUInt32 uiCollisionLayer,
    ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, ezUInt32 uiIgnoreShapeId = ezInvalidIndex)
    : m_uiCollisionLayer(uiCollisionLayer)
    , m_ShapeTypes(shapeTypes)
    , m_uiIgnoreShapeId(uiIgnoreShapeId)
  {
  }

  ezUInt32 m_uiCollisionLayer = 0;
  ezBitflags<ezPhysicsShapeType> m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;
  ezUInt32 m_uiIgnoreShapeId = ezInvalidIndex;
  bool m_bIgnoreInitialOverlap = false;
};

enum class ezPhysicsHitCollection
{
  Closest,
  Any
};

class EZ_CORE_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

protected:
  ezPhysicsWorldModuleInterface(ezWorld* pWorld)
    : ezWorldModule(pWorld)
  {
  }

public:
  virtual bool Raycast(ezPhysicsCastResult& out_Result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_Results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_Result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_Result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(ezPhysicsOverlapResultArray& out_Results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const = 0;

  virtual ezVec3 GetGravity() const = 0;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize) {}
};

/// \brief Used to apply a physical impulse on the object
struct EZ_CORE_DLL ezMsgPhysicsAddImpulse : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddImpulse, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vImpulse;
  ezUInt32 m_uiShapeId = ezInvalidIndex;
};

/// \brief Used to apply a physical force on the object
struct EZ_CORE_DLL ezMsgPhysicsAddForce : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddForce, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vForce;
};

struct EZ_CORE_DLL ezMsgPhysicsJointBroke : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsJointBroke, ezEventMessage);

  ezGameObjectHandle m_hJointObject;
};


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Communication/Message.h>

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
