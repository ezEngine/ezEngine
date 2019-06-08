#include <RecastPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Recast/DetourNavMesh.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <Recast/Recast.h>
#include <Recast/RecastAlloc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecastNavMeshResource, 1, ezRTTIDefaultAllocator<ezRecastNavMeshResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezRecastNavMeshResource);
// clang-format on


ezRecastNavMeshResourceDescriptor::ezRecastNavMeshResourceDescriptor() = default;
ezRecastNavMeshResourceDescriptor::ezRecastNavMeshResourceDescriptor(ezRecastNavMeshResourceDescriptor&& rhs) = default;

ezRecastNavMeshResourceDescriptor::~ezRecastNavMeshResourceDescriptor() = default;

void ezRecastNavMeshResourceDescriptor::Clear()
{
  m_DetourNavmeshData.Clear();
  m_pNavMeshPolygons.Clear();
}

void ezRecastNavMeshResourceDescriptor::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);
  stream.WriteArray(m_DetourNavmeshData);

  const bool hasPolygons = m_pNavMeshPolygons != nullptr;
  stream << hasPolygons;

  if (hasPolygons)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    const auto& mesh = *m_pNavMeshPolygons.Borrow();

    stream << (int)mesh.nverts;
    stream << (int)mesh.npolys;
    stream << (int)mesh.maxpolys;
    stream << (int)mesh.nvp;
    stream << (float)mesh.bmin[0];
    stream << (float)mesh.bmin[1];
    stream << (float)mesh.bmin[2];
    stream << (float)mesh.bmax[0];
    stream << (float)mesh.bmax[1];
    stream << (float)mesh.bmax[2];
    stream << (float)mesh.cs;
    stream << (float)mesh.ch;
    stream << (int)mesh.borderSize;
    stream << (float)mesh.maxEdgeError;

    stream.WriteBytes(mesh.verts, sizeof(ezUInt16) * mesh.nverts * 3);
    stream.WriteBytes(mesh.polys, sizeof(ezUInt16) * mesh.maxpolys * mesh.nvp * 2);
    stream.WriteBytes(mesh.regs, sizeof(ezUInt16) * mesh.maxpolys);
    stream.WriteBytes(mesh.flags, sizeof(ezUInt16) * mesh.maxpolys);
    stream.WriteBytes(mesh.areas, sizeof(ezUInt8) * mesh.maxpolys);
  }
}

void ezRecastNavMeshResourceDescriptor::Deserialize(ezStreamReader& stream)
{
  const ezTypeVersion version = stream.ReadVersion(1);
  stream.ReadArray(m_DetourNavmeshData);

  bool hasPolygons = false;
  stream >> hasPolygons;

  m_pNavMeshPolygons = nullptr;

  if (hasPolygons)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    m_pNavMeshPolygons = EZ_DEFAULT_NEW(rcPolyMesh);

    auto& mesh = *m_pNavMeshPolygons.Borrow();

    stream >> mesh.nverts;
    stream >> mesh.npolys;
    stream >> mesh.maxpolys;
    stream >> mesh.nvp;
    stream >> mesh.bmin[0];
    stream >> mesh.bmin[1];
    stream >> mesh.bmin[2];
    stream >> mesh.bmax[0];
    stream >> mesh.bmax[1];
    stream >> mesh.bmax[2];
    stream >> mesh.cs;
    stream >> mesh.ch;
    stream >> mesh.borderSize;
    stream >> mesh.maxEdgeError;

    mesh.verts = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.nverts * 3, RC_ALLOC_PERM);
    mesh.polys = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.npolys * mesh.nvp * 2, RC_ALLOC_PERM);
    mesh.regs = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.npolys, RC_ALLOC_PERM);
    mesh.areas = (ezUInt8*)rcAlloc(sizeof(ezUInt8) * mesh.npolys, RC_ALLOC_PERM);

    stream.ReadBytes(mesh.verts, sizeof(ezUInt16) * mesh.nverts * 3);
    stream.ReadBytes(mesh.polys, sizeof(ezUInt16) * mesh.maxpolys * mesh.nvp * 2);
    stream.ReadBytes(mesh.regs, sizeof(ezUInt16) * mesh.maxpolys);
    stream.ReadBytes(mesh.flags, sizeof(ezUInt16) * mesh.maxpolys);
    stream.ReadBytes(mesh.areas, sizeof(ezUInt8) * mesh.maxpolys);
  }
}

//////////////////////////////////////////////////////////////////////////

ezRecastNavMeshResource::ezRecastNavMeshResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezRecastNavMeshResource);
}

ezRecastNavMeshResource::~ezRecastNavMeshResource() = default;

ezResourceLoadDesc ezRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_pNavMesh.Clear();

  return res;
}

ezResourceLoadDesc ezRecastNavMeshResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezRecastNavMeshResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  ezRecastNavMeshResourceDescriptor desc;
  desc.Deserialize(*Stream);

  CreateResource(std::move(desc));

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezRecastNavMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezRecastNavMeshResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_DetourNavmeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMesh != nullptr ? sizeof(dtNavMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMeshPolygons != nullptr ? sizeof(rcPolyMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezRecastNavMeshResource, ezRecastNavMeshResourceDescriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  m_pNavMeshPolygons = std::move(descriptor.m_pNavMeshPolygons);
  m_DetourNavmeshData = std::move(descriptor.m_DetourNavmeshData);

  m_pNavMesh = EZ_DEFAULT_NEW(dtNavMesh);

  // the dtNavMesh does not need to free the data, the resource owns it
  const int dtMeshFlags = 0;
  m_pNavMesh->init(m_DetourNavmeshData.GetData(), m_DetourNavmeshData.GetCount(), dtMeshFlags);

  return res;
}
