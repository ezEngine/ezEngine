#include <RecastPlugin/RecastPluginPCH.h>

#include <DetourNavMesh.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Recast.h>
#include <RecastAlloc.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecastNavMeshResource, 1, ezRTTIDefaultAllocator<ezRecastNavMeshResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezRecastNavMeshResource);
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezRecastNavMeshResourceDescriptor::ezRecastNavMeshResourceDescriptor() = default;
ezRecastNavMeshResourceDescriptor::ezRecastNavMeshResourceDescriptor(ezRecastNavMeshResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

ezRecastNavMeshResourceDescriptor::~ezRecastNavMeshResourceDescriptor()
{
  Clear();
}

void ezRecastNavMeshResourceDescriptor::operator=(ezRecastNavMeshResourceDescriptor&& rhs)
{
  m_DetourNavmeshData = std::move(rhs.m_DetourNavmeshData);

  m_pNavMeshPolygons = rhs.m_pNavMeshPolygons;
  rhs.m_pNavMeshPolygons = nullptr;
}

void ezRecastNavMeshResourceDescriptor::Clear()
{
  m_DetourNavmeshData.Clear();
  EZ_DEFAULT_DELETE(m_pNavMeshPolygons);
}

//////////////////////////////////////////////////////////////////////////

ezResult ezRecastNavMeshResourceDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_DetourNavmeshData));

  const bool hasPolygons = m_pNavMeshPolygons != nullptr;
  inout_stream << hasPolygons;

  if (hasPolygons)
  {
    static_assert(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    const auto& mesh = *m_pNavMeshPolygons;

    inout_stream << (int)mesh.nverts;
    inout_stream << (int)mesh.npolys;
    inout_stream << (int)mesh.npolys; // do not use mesh.maxpolys
    inout_stream << (int)mesh.nvp;
    inout_stream << (float)mesh.bmin[0];
    inout_stream << (float)mesh.bmin[1];
    inout_stream << (float)mesh.bmin[2];
    inout_stream << (float)mesh.bmax[0];
    inout_stream << (float)mesh.bmax[1];
    inout_stream << (float)mesh.bmax[2];
    inout_stream << (float)mesh.cs;
    inout_stream << (float)mesh.ch;
    inout_stream << (int)mesh.borderSize;
    inout_stream << (float)mesh.maxEdgeError;

    EZ_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.verts, sizeof(ezUInt16) * mesh.nverts * 3));
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.polys, sizeof(ezUInt16) * mesh.npolys * mesh.nvp * 2));
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.regs, sizeof(ezUInt16) * mesh.npolys));
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.flags, sizeof(ezUInt16) * mesh.npolys));
    EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(mesh.areas, sizeof(ezUInt8) * mesh.npolys));
  }

  return EZ_SUCCESS;
}

ezResult ezRecastNavMeshResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  Clear();

  const ezTypeVersion version = inout_stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_DetourNavmeshData));

  bool hasPolygons = false;
  inout_stream >> hasPolygons;

  if (hasPolygons)
  {
    static_assert(sizeof(rcPolyMesh) == sizeof(void*) * 5 + sizeof(int) * 14, "rcPolyMesh data structure has changed");

    m_pNavMeshPolygons = EZ_DEFAULT_NEW(rcPolyMesh);

    auto& mesh = *m_pNavMeshPolygons;

    inout_stream >> mesh.nverts;
    inout_stream >> mesh.npolys;
    inout_stream >> mesh.maxpolys;
    inout_stream >> mesh.nvp;
    inout_stream >> mesh.bmin[0];
    inout_stream >> mesh.bmin[1];
    inout_stream >> mesh.bmin[2];
    inout_stream >> mesh.bmax[0];
    inout_stream >> mesh.bmax[1];
    inout_stream >> mesh.bmax[2];
    inout_stream >> mesh.cs;
    inout_stream >> mesh.ch;
    inout_stream >> mesh.borderSize;
    inout_stream >> mesh.maxEdgeError;

    EZ_ASSERT_DEBUG(mesh.maxpolys >= mesh.npolys, "Invalid navmesh polygon count");

    mesh.verts = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.nverts * 3, RC_ALLOC_PERM);
    mesh.polys = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.maxpolys * mesh.nvp * 2, RC_ALLOC_PERM);
    mesh.regs = (ezUInt16*)rcAlloc(sizeof(ezUInt16) * mesh.maxpolys, RC_ALLOC_PERM);
    mesh.areas = (ezUInt8*)rcAlloc(sizeof(ezUInt8) * mesh.maxpolys, RC_ALLOC_PERM);

    inout_stream.ReadBytes(mesh.verts, sizeof(ezUInt16) * mesh.nverts * 3);
    inout_stream.ReadBytes(mesh.polys, sizeof(ezUInt16) * mesh.maxpolys * mesh.nvp * 2);
    inout_stream.ReadBytes(mesh.regs, sizeof(ezUInt16) * mesh.maxpolys);
    inout_stream.ReadBytes(mesh.flags, sizeof(ezUInt16) * mesh.maxpolys);
    inout_stream.ReadBytes(mesh.areas, sizeof(ezUInt8) * mesh.maxpolys);
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezRecastNavMeshResource::ezRecastNavMeshResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(ezRecastNavMeshResource);
}

ezRecastNavMeshResource::~ezRecastNavMeshResource()
{
  EZ_DEFAULT_DELETE(m_pNavMeshPolygons);
  EZ_DEFAULT_DELETE(m_pNavMesh);
}

ezResourceLoadDesc ezRecastNavMeshResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_DetourNavmeshData.Clear();
  m_DetourNavmeshData.Compact();
  EZ_DEFAULT_DELETE(m_pNavMesh);
  EZ_DEFAULT_DELETE(m_pNavMeshPolygons);

  return res;
}

ezResourceLoadDesc ezRecastNavMeshResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezRecastNavMeshResource::UpdateContent", GetResourceIdOrDescription());

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
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezRecastNavMeshResourceDescriptor descriptor;
  descriptor.Deserialize(*Stream).IgnoreResult();

  return CreateResource(std::move(descriptor));
}

void ezRecastNavMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezRecastNavMeshResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_DetourNavmeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMesh != nullptr ? sizeof(dtNavMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryCPU += m_pNavMeshPolygons != nullptr ? sizeof(rcPolyMesh) : 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezRecastNavMeshResource::CreateResource(ezRecastNavMeshResourceDescriptor&& descriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  m_pNavMeshPolygons = descriptor.m_pNavMeshPolygons;
  descriptor.m_pNavMeshPolygons = nullptr;

  m_DetourNavmeshData = std::move(descriptor.m_DetourNavmeshData);

  if (!m_DetourNavmeshData.IsEmpty())
  {
    m_pNavMesh = EZ_DEFAULT_NEW(dtNavMesh);

    // the dtNavMesh does not need to free the data, the resource owns it
    const int dtMeshFlags = 0;
    m_pNavMesh->init(m_DetourNavmeshData.GetData(), m_DetourNavmeshData.GetCount(), dtMeshFlags);
  }

  return res;
}
