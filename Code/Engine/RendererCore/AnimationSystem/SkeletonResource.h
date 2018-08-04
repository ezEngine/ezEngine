#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Basics.h>

struct ezSkeletonResourceGeometry
{
  // scale is used to resize a unit sphere / box / capsule
  ezTransform m_Transform;
  ezUInt16 m_uiAttachedToBone = 0;
  ezEnum<ezSkeletonBoneGeometryType> m_Type;
};

struct EZ_RENDERERCORE_DLL ezSkeletonResourceDescriptor
{
  ezSkeleton m_Skeleton;
  ezDynamicArray<ezSkeletonResourceGeometry> m_Geometry;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

class EZ_RENDERERCORE_DLL ezSkeletonResource : public ezResource<ezSkeletonResource, ezSkeletonResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonResource, ezResourceBase);

public:
  ezSkeletonResource();
  ~ezSkeletonResource();

  const ezSkeletonResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual ezResourceLoadDesc CreateResource(const ezSkeletonResourceDescriptor& descriptor) override;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezSkeletonResourceDescriptor m_Descriptor;
};
