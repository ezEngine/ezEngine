#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>


using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

/// \brief Classifies the facing of an individual raycast hit
enum class ezPhysicsHitType : int8_t
{
  Undefined = -1,        ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0, ///< The raycast hit the front face of a triangle
  TriangleBackFace = 1,  ///< The raycast hit the back face of a triangle
};

/// \brief Used for raycast and sweep tests
struct ezPhysicsCastResult
{
  ezVec3 m_vPosition;
  ezVec3 m_vNormal;
  float m_fDistance;

  ezGameObjectHandle m_hShapeObject;                        ///< The game object to which the hit physics shape is attached.
  ezGameObjectHandle m_hActorObject;                        ///< The game object to which the parent actor of the hit physics shape is attached.
  ezSurfaceResourceHandle m_hSurface;                       ///< The type of surface that was hit (if available)
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;             ///< An ID either per object (rigid-body / ragdoll) or per shape (implementation specific) that can be used to ignore this object during raycasts and shape queries.
  ezPhysicsHitType m_hitType = ezPhysicsHitType::Undefined; ///< Classification of the triangle face, see ezPhysicsHitType

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct ezPhysicsCastResultArray
{
  ezHybridArray<ezPhysicsCastResult, 16> m_Results;
};

/// \brief Used to report overlap query results
struct ezPhysicsOverlapResult
{
  EZ_DECLARE_POD_TYPE();

  ezGameObjectHandle m_hShapeObject;            ///< The game object to which the hit physics shape is attached.
  ezGameObjectHandle m_hActorObject;            ///< The game object to which the parent actor of the hit physics shape is attached.
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex; ///< The shape id of the hit physics shape
  ezVec3 m_vCenterPosition;                     ///< The center position of the reported object in world space.

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct ezPhysicsOverlapResultArray
{
  ezHybridArray<ezPhysicsOverlapResult, 16> m_Results;
};

/// \brief Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
///
/// This is mainly for optimization purposes. It is up to the physics integration to support some or all of these flags.
///
/// Note: If this is modified, 'Physics.ts' also has to be updated.
EZ_DECLARE_FLAGS_WITH_DEFAULT(ezUInt32, ezPhysicsShapeType, 0xFFFFFFFF,
  Static,    ///< Static geometry
  Dynamic,   ///< Dynamic and kinematic objects
  Query,     ///< Query shapes are kinematic bodies that don't participate in the simulation and are only used for raycasts and other queries.
  Trigger,   ///< Trigger shapes
  Character, ///< Shapes associated with character controllers.
  Ragdoll,   ///< All shapes belonging to ragdolls.
  Rope,      ///< All shapes belonging to ropes.
  Cloth      ///< Soft-body shapes. Mainly for decorative purposes.
);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezPhysicsShapeType);

struct ezPhysicsQueryParameters
{
  ezPhysicsQueryParameters() = default;
  explicit ezPhysicsQueryParameters(ezUInt32 uiCollisionLayer,
    ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Default, ezUInt32 uiIgnoreObjectFilterID = ezInvalidIndex)
    : m_uiCollisionLayer(uiCollisionLayer)
    , m_ShapeTypes(shapeTypes)
    , m_uiIgnoreObjectFilterID(uiIgnoreObjectFilterID)
  {
  }

  ezUInt32 m_uiCollisionLayer = 0;
  ezBitflags<ezPhysicsShapeType> m_ShapeTypes = ezPhysicsShapeType::Default;
  ezUInt32 m_uiIgnoreObjectFilterID = ezInvalidIndex;
  bool m_bIgnoreInitialOverlap = false;
};

enum class ezPhysicsHitCollection
{
  Closest,
  Any
};
