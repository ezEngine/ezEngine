#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class ezWorld;
class ezGameObject;

/// Script extension class providing physics world queries and utilities for scripts.
///
/// Exposes physics system functionality to scripts including collision detection,
/// raycasting, and shape overlap testing. All functions require a valid world
/// and may return no results if no physics world module is active.
class EZ_CORE_DLL ezScriptExtensionClass_Physics
{
public:
  /// Gets the current gravity vector for the physics world.
  static ezVec3 GetGravity(ezWorld* pWorld);

  /// Finds collision layer index by name, returns invalid index if not found.
  static ezUInt8 GetCollisionLayerByName(ezWorld* pWorld, ezStringView sLayerName);

  /// Finds weight category index by name, returns invalid key if not found.
  static ezUInt8 GetWeightCategoryByName(ezWorld* pWorld, ezStringView sCategoryName);

  /// Finds impulse type index by name, returns invalid key if not found.
  static ezUInt8 GetImpulseTypeByName(ezWorld* pWorld, ezStringView sImpulseTypeName);

  /// Performs raycast and returns hit information if collision is found.
  static bool Raycast(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vDirection, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);

  /// Tests if a line segment intersects with any physics shapes.
  static bool OverlapTestLine(ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);

  /// Tests if a sphere at the given position overlaps with any physics shapes.
  static bool OverlapTestSphere(ezWorld* pWorld, float fRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  /// Tests if a capsule with the given transform overlaps with any physics shapes.
  static bool OverlapTestCapsule(ezWorld* pWorld, float fRadius, float fHeight, const ezTransform& transform, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  /// Sweeps a sphere along a direction and returns hit information if collision is found.
  static bool SweepTestSphere(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, float fRadius, const ezVec3& vStart, const ezVec3& vDirection, float fDistance, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  /// Sweeps a capsule along a direction and returns hit information if collision is found.
  static bool SweepTestCapsule(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, float fRadius, float fHeight, const ezTransform& start, const ezVec3& vDirection, float fDistance, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  /// Performs raycast and triggers surface interaction at hit point if collision is found.
  static bool RaycastSurfaceInteraction(ezWorld* pWorld, const ezVec3& vRayStart, const ezVec3& vRayDirection, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, ezStringView sFallbackSurface, const ezTempHashedString& sInteraction, float fInteractionImpulse, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Physics);
