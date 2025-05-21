#pragma once

#include <Core/CoreDLL.h>
#include <Core/Interfaces/PhysicsQuery.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>

class ezWorld;
class ezGameObject;

class EZ_CORE_DLL ezScriptExtensionClass_Physics
{
public:
  static ezVec3 GetGravity(ezWorld* pWorld);

  static ezUInt8 GetCollisionLayerByName(ezWorld* pWorld, ezStringView sLayerName);
  static ezUInt8 GetWeightCategoryByName(ezWorld* pWorld, ezStringView sCategoryName);
  static ezUInt8 GetImpulseTypeByName(ezWorld* pWorld, ezStringView sImpulseTypeName);

  static bool Raycast(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vDirection, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);

  static bool OverlapTestLine(ezWorld* pWorld, const ezVec3& vStart, const ezVec3& vEnd, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);

  static bool OverlapTestSphere(ezWorld* pWorld, float fRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  static bool OverlapTestCapsule(ezWorld* pWorld, float fRadius, float fHeight, const ezTransform& transform, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  static bool SweepTestSphere(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, float fRadius, const ezVec3& vStart, const ezVec3& vDirection, float fDistance, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  static bool SweepTestCapsule(ezVec3& out_vHitPosition, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitObject, ezWorld* pWorld, float fRadius, float fHeight, const ezTransform& start, const ezVec3& vDirection, float fDistance, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic);

  static bool RaycastSurfaceInteraction(ezWorld* pWorld, const ezVec3& vRayStart, const ezVec3& vRayDirection, ezUInt8 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, ezStringView sFallbackSurface, const ezTempHashedString& sInteraction, float fInteractionImpulse, ezUInt32 uiIgnoreObjectID = ezInvalidIndex);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Physics);
