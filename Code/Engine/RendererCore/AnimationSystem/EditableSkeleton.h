#pragma once

#include <RendererCore/Basics.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Declarations.h>

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSkeletonJointGeometryType);

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
  ezTransform m_Transform;
  ezEnum<ezSkeletonJointGeometryType> m_Geometry;

  float m_fLength = 0;    // Box, Capsule, 0 means parent joint to this joint
  float m_fWidth = 0;     // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
};

class EZ_RENDERERCORE_DLL ezEditableSkeleton : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditableSkeleton, ezReflectedClass);

public:
  ezEditableSkeleton();
  ~ezEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(ezSkeletonResourceDescriptor& desc);

  ezString m_sAnimationFile;

  ezEnum<ezBasisAxis> m_ForwardDir;
  ezEnum<ezBasisAxis> m_RightDir;
  ezEnum<ezBasisAxis> m_UpDir;
  float m_fUniformScaling;

  ezHybridArray<ezEditableSkeletonJoint*, 4> m_Children;
};
