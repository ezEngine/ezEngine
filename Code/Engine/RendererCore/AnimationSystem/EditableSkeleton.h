#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Declarations.h>

class ezSkeletonBuilder;
class ezSkeleton;

namespace ozz::animation
{
  class Skeleton;

  namespace offline
  {
    struct RawSkeleton;
  }
} // namespace ozz::animation

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSkeletonJointGeometryType);

struct EZ_RENDERERCORE_DLL ezEditableSkeletonBoneShape : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonBoneShape, ezReflectedClass);

  ezEnum<ezSkeletonJointGeometryType> m_Geometry;

  ezVec3 m_vOffset = ezVec3::MakeZero();
  ezQuat m_qRotation = ezQuat::MakeIdentity();

  float m_fLength = 0;    // Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth = 0;     // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius
};

struct EZ_RENDERERCORE_DLL ezEditableSkeletonBoneCollider : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonBoneCollider, ezReflectedClass);

  ezString m_sIdentifier;
  ezDynamicArray<ezVec3> m_VertexPositions;
  ezDynamicArray<ezUInt8> m_TriangleIndices;
};

class EZ_RENDERERCORE_DLL ezEditableSkeletonJoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonJoint, ezReflectedClass);

public:
  ezEditableSkeletonJoint();
  ~ezEditableSkeletonJoint();

  const char* GetName() const;
  void SetName(const char* szSz);

  void ClearJoints();

  // copies the properties for geometry etc. from another joint
  // does NOT copy the name, the transform or the children
  void CopyPropertiesFrom(const ezEditableSkeletonJoint* pJoint);

  ezHashedString m_sName;
  ezTransform m_LocalTransform = ezTransform::MakeIdentity();

  ezEnum<ezSkeletonJointType> m_JointType;

  float m_fStiffness = 0.0f;

  ezAngle m_TwistLimitHalfAngle;
  ezAngle m_TwistLimitCenterAngle;
  ezAngle m_SwingLimitY;
  ezAngle m_SwingLimitZ;

  ezVec3 m_vGizmoOffsetPositionRO = ezVec3::MakeZero();
  ezQuat m_qGizmoOffsetRotationRO = ezQuat::MakeIdentity();

  ezQuat m_qLocalJointRotation = ezQuat::MakeIdentity();

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
  ezHybridArray<ezEditableSkeletonBoneShape, 1> m_BoneShapes;
  ezDynamicArray<ezEditableSkeletonBoneCollider> m_BoneColliders;

  bool m_bOverrideSurface = false;
  bool m_bOverrideCollisionLayer = false;
  ezString m_sSurfaceOverride;
  ezUInt8 m_uiCollisionLayerOverride;
};

class EZ_RENDERERCORE_DLL ezEditableSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeleton, ezReflectedClass);

public:
  ezEditableSkeleton();
  ~ezEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(ezSkeletonResourceDescriptor& ref_desc) const;
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const;
  void GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const;
  void CreateJointsRecursive(ezSkeletonBuilder& ref_sb, ezSkeletonResourceDescriptor& ref_desc, const ezEditableSkeletonJoint* pParentJoint, const ezEditableSkeletonJoint* pThisJoint, ezUInt16 uiThisJointIdx, const ezQuat& qParentAccuRot, const ezMat4& mRootTransform) const;

  ezString m_sSourceFile;
  ezString m_sPreviewMesh;

  ezString m_sSurfaceFile;
  ezUInt8 m_uiCollisionLayer = 0;

  float m_fUniformScaling = 1.0f;
  float m_fMaxImpulse = 100.0f;

  ezEnum<ezMeshImportTransform> m_ImportTransform;
  ezEnum<ezBasisAxis> m_RightDir = ezBasisAxis::NegativeX;
  ezEnum<ezBasisAxis> m_UpDir = ezBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;
  ezEnum<ezBasisAxis> m_BoneDirection;

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
};

struct EZ_RENDERERCORE_DLL ezExposedBone
{
  ezString m_sName;
  ezString m_sParent;
  ezTransform m_Transform;
  // when adding new values, the hash function below has to be adjusted
};

EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezExposedBone);

EZ_RENDERERCORE_DLL void operator<<(ezStreamWriter& inout_stream, const ezExposedBone& bone);
EZ_RENDERERCORE_DLL void operator>>(ezStreamReader& inout_stream, ezExposedBone& ref_bone);
EZ_RENDERERCORE_DLL bool operator==(const ezExposedBone& lhs, const ezExposedBone& rhs);

template <>
struct ezHashHelper<ezExposedBone>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezExposedBone& value)
  {
    return ezHashingUtils::xxHash32String(value.m_sName) + ezHashingUtils::xxHash32String(value.m_sParent) + ezHashingUtils::xxHash32(&value, sizeof(ezTransform));
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezExposedBone& a, const ezExposedBone& b) { return a == b; }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezExposedBone);
