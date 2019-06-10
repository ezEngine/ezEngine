#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct rcPolyMesh;
class dtNavMesh;

typedef ezTypedResourceHandle<class ezRecastNavMeshResource> ezRecastNavMeshResourceHandle;

struct EZ_RECASTPLUGIN_DLL ezRecastNavMeshResourceDescriptor
{
  ezRecastNavMeshResourceDescriptor();
  ezRecastNavMeshResourceDescriptor(const ezRecastNavMeshResourceDescriptor& rhs) = delete;
  ezRecastNavMeshResourceDescriptor(ezRecastNavMeshResourceDescriptor&& rhs);
  ~ezRecastNavMeshResourceDescriptor();
  void operator=(ezRecastNavMeshResourceDescriptor&& rhs);
  void operator=(const ezRecastNavMeshResourceDescriptor& rhs) = delete;

  /// \brief Data that was created by dtCreateNavMeshData() and will be used for dtNavMesh::init()
  ezDataBuffer m_DetourNavmeshData;

  /// \brief Optional, if available the navmesh can be visualized at runtime
  rcPolyMesh* m_pNavMeshPolygons = nullptr;

  void Clear();

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

class EZ_RECASTPLUGIN_DLL ezRecastNavMeshResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastNavMeshResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRecastNavMeshResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRecastNavMeshResource, ezRecastNavMeshResourceDescriptor);

public:
  ezRecastNavMeshResource();
  ~ezRecastNavMeshResource();

  const dtNavMesh* GetNavMesh() const { return m_pNavMesh; }
  const rcPolyMesh* GetNavMeshPolygons() const { return m_pNavMeshPolygons; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezDataBuffer m_DetourNavmeshData;
  dtNavMesh* m_pNavMesh = nullptr;
  rcPolyMesh* m_pNavMeshPolygons = nullptr;
};
