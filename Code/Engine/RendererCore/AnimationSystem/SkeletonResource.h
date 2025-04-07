#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using ezSurfaceResourceHandle = ezTypedResourceHandle<class ezSurfaceResource>;

struct ezSkeletonResourceGeometry
{
  // scale is used to resize a unit sphere / box / capsule
  ezTransform m_Transform;
  ezUInt16 m_uiAttachedToJoint = 0;
  ezEnum<ezSkeletonJointGeometryType> m_Type;

  // for convex geometry
  ezDynamicArray<ezVec3> m_VertexPositions;
  ezDynamicArray<ezUInt8> m_TriangleIndices;
};

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

  // used for motion extraction
  ezUInt16 m_uiLeftFootJoint = ezInvalidJointIndex;
  ezUInt16 m_uiRightFootJoint = ezInvalidJointIndex;

  ezDynamicArray<ezSkeletonResourceGeometry> m_Geometry;
};

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

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
