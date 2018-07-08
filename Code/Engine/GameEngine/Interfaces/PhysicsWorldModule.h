#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/WorldModule.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>

struct ezGameObjectHandle;

typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

/// \brief Used for raycast and seep tests
struct ezPhysicsHitResult
{
  ezVec3 m_vPosition;
  ezVec3 m_vNormal;
  float m_fDistance;

  ezGameObjectHandle m_hShapeObject; ///< The game object to which the hit physics shape is attached.
  ezGameObjectHandle m_hActorObject; ///< The game object to which the parent actor of the hit physics shape is attached.
  ezSurfaceResourceHandle m_hSurface;
  ezUInt32 m_uiShapeId; ///< The shape id of the hit physics shape
};

/// \brief Used to report overlap query results
struct ezPhysicsOverlapResult
{
  struct Hit
  {
    ezGameObjectHandle m_hShapeObject; ///< The game object to which the hit physics shape is attached.
    ezGameObjectHandle m_hActorObject; ///< The game object to which the parent actor of the hit physics shape is attached.
    ezUInt32 m_uiShapeId; ///< The shape id of the hit physics shape
  };

  ezDynamicArray<Hit> m_Results;
};

class EZ_GAMEENGINE_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

protected:
  ezPhysicsWorldModuleInterface(ezWorld* pWorld) : ezWorldModule(pWorld) { }

public:

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
    ezPhysicsHitResult& out_HitResult, bool bIncludeDynamic = true, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual bool SweepTestSphere(float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual bool SweepTestBox(ezVec3 vBoxExtends, const ezTransform& start, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual bool SweepTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& start, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;
  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual void QueryDynamicShapesInSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezPhysicsOverlapResult& out_Results, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const = 0;

  virtual ezVec3 GetGravity() const = 0;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize) {}

private:

};

/// \brief Used to apply a physical impulse on the object
struct EZ_GAMEENGINE_DLL ezMsgPhysicsAddImpulse : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddImpulse, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vImpulse;
  ezUInt32 m_uiShapeId = ezInvalidIndex;
};

/// \brief Used to apply a physical force on the object
struct EZ_GAMEENGINE_DLL ezMsgPhysicsAddForce : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPhysicsAddForce, ezMessage);

  ezVec3 m_vGlobalPosition;
  ezVec3 m_vForce;
};

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Communication/Message.h>

struct EZ_GAMEENGINE_DLL ezSmcTriangle
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiVertexIndices[3];
};

struct EZ_GAMEENGINE_DLL ezSmcSubMesh
{
  EZ_DECLARE_POD_TYPE();

  ezUInt32 m_uiFirstTriangle = 0;
  ezUInt32 m_uiNumTriangles = 0;
  ezUInt16 m_uiSurfaceIndex = 0;
};

struct EZ_GAMEENGINE_DLL ezSmcDescription
{
  ezDeque<ezVec3> m_Vertices;
  ezDeque<ezSmcTriangle> m_Triangles;
  ezDeque<ezSmcSubMesh> m_SubMeshes;
  ezDeque<ezString> m_Surfaces;
};

struct EZ_GAMEENGINE_DLL ezMsgBuildStaticMesh : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgBuildStaticMesh, ezMessage);

  /// \brief Append data to this description to add meshes to the automatic static mesh generation
  ezSmcDescription* m_pStaticMeshDescription;
};

