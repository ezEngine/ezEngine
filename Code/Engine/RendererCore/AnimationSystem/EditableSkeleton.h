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

/// \brief Describes the collision geometry shape for a bone.
///
/// Used during skeleton editing to define physics collision shapes for bones.
/// The shape can be a box, capsule, or sphere. If m_fLength is 0, it automatically
/// uses the distance from the parent joint to this joint.
struct EZ_RENDERERCORE_DLL ezEditableSkeletonBoneShape : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonBoneShape, ezReflectedClass);

  ezEnum<ezSkeletonJointGeometryType> m_Geometry;

  ezVec3 m_vOffset = ezVec3::MakeZero();
  ezQuat m_qRotation = ezQuat::MakeIdentity();

  float m_fLength = 0;    ///< Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth = 0;     ///< Box
  float m_fThickness = 0; ///< Sphere radius, Capsule radius
};

/// \brief Defines a convex mesh collider for a bone.
///
/// Stores vertex positions and triangle indices that define a convex collision mesh
/// for a bone. Used when more complex collision shapes than simple primitives are needed.
struct EZ_RENDERERCORE_DLL ezEditableSkeletonBoneCollider : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonBoneCollider, ezReflectedClass);

  ezString m_sIdentifier;
  ezDynamicArray<ezVec3> m_VertexPositions;
  ezDynamicArray<ezUInt8> m_TriangleIndices;
};

/// \brief Represents a single joint in an editable skeleton.
///
/// Used during skeleton editing and import. Contains the joint's transform, collision shapes,
/// physics properties like joint limits and stiffness, and references to child joints.
/// This is the editable representation which gets converted to the runtime ezSkeletonJoint.
class EZ_RENDERERCORE_DLL ezEditableSkeletonJoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonJoint, ezReflectedClass);

public:
  ezEditableSkeletonJoint();
  ~ezEditableSkeletonJoint();

  const char* GetName() const;
  void SetName(const char* szSz);

  void ClearJoints();

  /// \brief Copies the properties for geometry, physics, and collision from another joint.
  ///
  /// Does NOT copy the name, the transform or the children.
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

/// \brief Editable representation of a skeleton used in the editor and import pipeline.
///
/// Contains the full hierarchy of joints with all their properties. Used for skeleton editing,
/// mesh import, and for generating the runtime skeleton resource.
/// Can convert itself into ezSkeletonResourceDescriptor and ozz::animation::Skeleton formats.
class EZ_RENDERERCORE_DLL ezEditableSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeleton, ezReflectedClass);

public:
  ezEditableSkeleton();
  ~ezEditableSkeleton();

  void ClearJoints();

  /// \brief Fills the given resource descriptor with data from this editable skeleton.
  void FillResourceDescriptor(ezSkeletonResourceDescriptor& ref_desc) const;

  /// \brief Generates a raw ozz skeleton, which can then be compiled to the final ozz format.
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const;

  /// \brief Generates a fully compiled ozz skeleton directly.
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

  // used for motion extraction
  ezString m_sLeftFootJoint;
  ezString m_sRightFootJoint;
};

/// \brief Represents a bone that is exposed for external use, typically for attachments.
///
/// Used to define attachment points on a skeleton where other objects can be attached.
/// Stores the bone name, parent bone, and the transform relative to the parent.
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
