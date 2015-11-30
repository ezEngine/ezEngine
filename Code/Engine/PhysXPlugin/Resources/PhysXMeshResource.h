#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

namespace physx
{
  class PxTriangleMesh;
}

typedef ezResourceHandle<class ezPhysXMeshResource> ezPhysXMeshResourceHandle;

struct EZ_PHYSXPLUGIN_DLL ezPhysXMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class EZ_PHYSXPLUGIN_DLL ezPhysXMeshResource : public ezResource<ezPhysXMeshResource, ezPhysXMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXMeshResource, ezResourceBase);

public:
  ezPhysXMeshResource();
  ~ezPhysXMeshResource();

  physx::PxTriangleMesh* GetTriangleMesh() const { return m_pPxTriangleMesh; }
  physx::PxConvexMesh* GetConvexMesh() const { return m_pPxConvexMesh; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezPhysXMeshResourceDescriptor& descriptor) override;

private:
  physx::PxTriangleMesh* m_pPxTriangleMesh;
  physx::PxConvexMesh* m_pPxConvexMesh;
};

