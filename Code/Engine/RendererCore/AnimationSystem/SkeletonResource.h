#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

/// Defines a collision geometry shape attached to a joint in a skeleton.
///
/// Used for physics collision detection. The transform scales and positions a unit sphere,
/// box, or capsule. For convex meshes, the vertex positions and triangle indices define the shape.
struct ezSkeletonResourceGeometry
{
  ezTransform m_Transform; ///< Scale is used to resize a unit sphere / box / capsule
  ezUInt16 m_uiAttachedToJoint = 0;
  ezEnum<ezSkeletonJointGeometryType> m_Type;

  ezDynamicArray<ezVec3> m_VertexPositions;  ///< For convex geometry
  ezDynamicArray<ezUInt8> m_TriangleIndices; ///< For convex geometry
};

/// Descriptor containing all data needed to create a skeleton resource.
struct EZ_RENDERERCORE_DLL ezSkeletonResourceDescriptor
{
  ezSkeletonResourceDescriptor();
  ~ezSkeletonResourceDescriptor();
  ezSkeletonResourceDescriptor(const ezSkeletonResourceDescriptor& rhs) = delete;
  ezSkeletonResourceDescriptor(ezSkeletonResourceDescriptor&& rhs);
  void operator=(ezSkeletonResourceDescriptor&& rhs);
  void operator=(const ezSkeletonResourceDescriptor& rhs) = delete;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  ezUInt64 GetHeapMemoryUsage() const;

  ezTransform m_RootTransform = ezTransform::MakeIdentity();
  ezSkeleton m_Skeleton;
  float m_fMaxImpulse = ezMath::HighValue<float>();

  ezUInt16 m_uiLeftFootJoint = ezInvalidJointIndex;  ///< Used for motion extraction
  ezUInt16 m_uiRightFootJoint = ezInvalidJointIndex; ///< Used for motion extraction

  ezDynamicArray<ezSkeletonResourceGeometry> m_Geometry;
};

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

/// Runtime resource containing skeleton data used for skeletal animation.
///
/// Stores the joint hierarchy, transforms, collision geometry, and other data needed
/// for animating skeletal meshes. Created from a ezSkeletonResourceDescriptor.
/// The skeleton is used by animation components and can be queried for joint indices and transforms.
class EZ_RENDERERCORE_DLL ezSkeletonResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezSkeletonResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezSkeletonResource, ezSkeletonResourceDescriptor);

public:
  ezSkeletonResource();
  ~ezSkeletonResource();

  const ezSkeletonResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezUniquePtr<ezSkeletonResourceDescriptor> m_pDescriptor;
};
