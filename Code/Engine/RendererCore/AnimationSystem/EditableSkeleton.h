#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Declarations.h>

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

  ezVec3 m_vOffset = ezVec3::ZeroVector();
  ezQuat m_qRotation = ezQuat::IdentityQuaternion();

  float m_fLength = 0;    // Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth = 0;     // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius

  bool m_bOverrideName = false;
  bool m_bOverrideSurface = false;
  bool m_bOverrideCollisionLayer = false;

  ezString m_sNameOverride;
  ezString m_sSurfaceOverride;
  ezUInt8 m_uiCollisionLayerOverride;
};

class EZ_RENDERERCORE_DLL ezEditableSkeletonJoint : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeletonJoint, ezReflectedClass);

public:
  ezEditableSkeletonJoint();
  ~ezEditableSkeletonJoint();

  const char* GetName() const;
  void SetName(const char* sz);

  void ClearJoints();

  // copies the properties for geometry etc. from another joint
  // does NOT copy the name, the transform or the children
  void CopyPropertiesFrom(const ezEditableSkeletonJoint* pJoint);

  ezHashedString m_sName;
  ezTransform m_Transform = ezTransform::IdentityTransform();

  ezVec3 GetJointPosGlobal() const;
  ezQuat GetJointLimitOrientation() const;
  void SetJointLimitOrientation(ezQuat val);

  ezAngle m_TwistLimitLow;
  ezAngle m_TwistLimitHigh;
  ezAngle m_SwingLimitX;
  ezAngle m_SwingLimitY;

  ezQuat m_qJointLimitOrientation = ezQuat::IdentityQuaternion();
  ezVec3 m_vJointPosGlobal = ezVec3::ZeroVector();

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
  ezHybridArray<ezEditableSkeletonBoneShape, 1> m_BoneShapes;
};

class EZ_RENDERERCORE_DLL ezEditableSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeleton, ezReflectedClass);

public:
  ezEditableSkeleton();
  ~ezEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(ezSkeletonResourceDescriptor& desc) const;
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_Skeleton) const;
  void GenerateOzzSkeleton(ozz::animation::Skeleton& out_Skeleton) const;
  void AddChildJoints(ezSkeletonBuilder& sb, ezSkeletonResourceDescriptor& desc, const ezEditableSkeletonJoint* pParentJoint, const ezEditableSkeletonJoint* pJoint, ezUInt32 uiJointIdx) const;

  ezString m_sAnimationFile;
  ezString m_sSurfaceFile;
  ezUInt8 m_uiCollisionLayer = 0;

  float m_fUniformScaling = 1.0f;

  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;
  bool m_bFlipForwardDir = false;
  ezEnum<ezBasisAxis> m_BoneDirection;

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
};
