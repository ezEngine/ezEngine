#include <KrautPlugin/KrautPluginPCH.h>

#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Textures/Texture2DResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeResource, 1, ezRTTIDefaultAllocator<ezKrautTreeResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezKrautTreeResource);
// clang-format on

ezKrautTreeResource::ezKrautTreeResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  m_Details.m_Bounds = ezBoundingBoxSphere::MakeInvalid();
}

ezResourceLoadDesc ezKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  EZ_LOCK(m_LodMutex);

  m_Details.m_Bounds = ezBoundingBoxSphere::MakeInvalid();
  m_Materials.Clear();
  m_TreeLODs.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezKrautTreeResource::UpdateContent(ezStreamReader* Stream)
{
  // Tree meshes are generated at runtime; there is no stream-based loading path.
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = (Stream == nullptr) ? ezResourceState::LoadedResourceMissing : ezResourceState::Loaded;
  return res;
}

void ezKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezKrautTreeResource, ezKrautTreeResourceDescriptor)
{
  EZ_LOCK(m_LodMutex);

  m_TreeLODs.Clear();
  m_Details = descriptor.m_Details;
  m_Materials = descriptor.m_Materials;

  // Create skeleton slots: distance/type info only, no mesh data yet.
  // Meshes are generated on demand via SetLodMesh().
  for (ezUInt32 lodIdx = 0; lodIdx < descriptor.m_Lods.GetCount(); ++lodIdx)
  {
    const auto& lodSrc = descriptor.m_Lods[lodIdx];

    if (lodSrc.m_LodType != ezKrautLodType::Mesh)
      break;

    auto& lodDst = m_TreeLODs.ExpandAndGetRef();
    lodDst.m_LodType = lodSrc.m_LodType;
    lodDst.m_fMinLodDistance = lodSrc.m_fMinLodDistance;
    lodDst.m_fMaxLodDistance = lodSrc.m_fMaxLodDistance;
    lodDst.m_uiNumBones = lodSrc.m_uiNumBones;
    lodDst.m_State = ezKrautLodState::NotGenerated;
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezKrautTreeResource::SetDetails(const ezKrautTreeResourceDetails& details, ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> materials)
{
  EZ_LOCK(m_LodMutex);
  m_Details = details;
  m_Materials = materials;
}

void ezKrautTreeResource::SetLodMesh(ezUInt32 uiLodIndex, const ezKrautTreeResourceDescriptor::LodData& lodSrc, ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> materials)
{
  EZ_LOCK(m_LodMutex);

  if (uiLodIndex >= m_TreeLODs.GetCount())
    return;

  auto& lodDst = m_TreeLODs[uiLodIndex];

  if (lodDst.m_State == ezKrautLodState::Ready)
    return; // already set

  lodDst.m_Materials = materials;

  // Count triangles by material type
  lodDst.m_uiNumTrianglesBranch = 0;
  lodDst.m_uiNumTrianglesFrond = 0;
  lodDst.m_uiNumTrianglesLeaf = 0;

  for (const auto& subMesh : lodSrc.m_SubMeshes)
  {
    if (subMesh.m_uiMaterialIndex < materials.GetCount())
    {
      const ezUInt32 uiTris = subMesh.m_uiNumTriangles;
      switch (materials[subMesh.m_uiMaterialIndex].m_MaterialType)
      {
        case ezKrautMaterialType::Branch:
          lodDst.m_uiNumTrianglesBranch += uiTris;
          break;
        case ezKrautMaterialType::Frond:
          lodDst.m_uiNumTrianglesFrond += uiTris;
          break;
        case ezKrautMaterialType::Leaf:
          lodDst.m_uiNumTrianglesLeaf += uiTris;
          break;
        default:
          break;
      }
    }
  }

  const ezUInt32 uiNumVertices = lodSrc.m_Vertices.GetCount();
  const ezUInt32 uiNumTriangles = lodSrc.m_Triangles.GetCount();
  const ezUInt32 uiSubMeshes = lodSrc.m_SubMeshes.GetCount();

  ezMeshResourceDescriptor md;
  auto& buffer = md.MeshBufferDesc();

  buffer.AddCommonStreams();
  buffer.AddStream(ezMeshVertexStreamType::TexCoord1);

  const bool bFullPrecision = true;
  buffer.AddStream(ezMeshVertexStreamType::Color0, bFullPrecision);
  buffer.AddStream(ezMeshVertexStreamType::Color1, bFullPrecision);
  buffer.AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

  for (ezUInt32 v = 0; v < uiNumVertices; ++v)
  {
    const auto& vtx = lodSrc.m_Vertices[v];

    buffer.SetPosition(v, vtx.m_vPosition);
    buffer.SetTexCoord0(v, ezVec2(vtx.m_vTexCoord.x, vtx.m_vTexCoord.y));
    buffer.SetTexCoord1(v, ezVec2(vtx.m_vTexCoord.z, vtx.m_fAmbientOcclusion));
    buffer.SetNormal(v, vtx.m_vNormal);
    buffer.SetTangent(v, vtx.m_vTangent.GetAsVec4(1.0f));

    ezColor color;
    color.r = vtx.m_fBendAndFlutterStrength;
    color.g = (float)vtx.m_uiBranchLevel;
    color.b = ezMath::ColorByteToFloat(vtx.m_uiFlutterPhase);
    color.a = ezMath::ColorByteToFloat(vtx.m_uiColorVariation);

    buffer.SetColor0(v, color);
    buffer.SetColor1(v, ezColor(vtx.m_vBendAnchor.x, vtx.m_vBendAnchor.y, vtx.m_vBendAnchor.z, vtx.m_fAnchorBendStrength));
  }

  for (ezUInt32 t = 0; t < uiNumTriangles; ++t)
  {
    const auto& tri = lodSrc.m_Triangles[t];
    buffer.SetTriangleIndices(t, tri.m_uiVertexIndex[0], tri.m_uiVertexIndex[1], tri.m_uiVertexIndex[2]);
  }

  for (ezUInt32 sm = 0; sm < uiSubMeshes; ++sm)
  {
    const auto& subMesh = lodSrc.m_SubMeshes[sm];
    md.AddSubMesh(subMesh.m_uiNumTriangles, subMesh.m_uiFirstTriangle, subMesh.m_uiMaterialIndex);
  }

  md.ComputeBounds();

  for (ezUInt32 mat = 0; mat < materials.GetCount(); ++mat)
  {
    md.SetMaterial(mat, materials[mat].m_sMaterial);
  }

  ezStringBuilder sResName, sResDesc;
  sResName.SetFormat("{0}_{1}_LOD{2}", GetResourceID(), GetCurrentResourceChangeCounter(), uiLodIndex);

  if (GetResourceDescription().IsEmpty())
    sResDesc = sResName;
  else
    sResDesc.SetFormat("{0}_{1}_LOD{2}", GetResourceDescription(), GetCurrentResourceChangeCounter(), uiLodIndex);

  lodDst.m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sResName);

  if (!lodDst.m_hMesh.IsValid())
    lodDst.m_hMesh = ezResourceManager::GetOrCreateResource<ezMeshResource>(sResName, std::move(md), sResDesc);

  lodDst.m_State = ezKrautLodState::Ready;
}

void ezKrautTreeResource::SetLodState(ezUInt32 uiLodIndex, ezKrautLodState state)
{
  EZ_LOCK(m_LodMutex);

  if (uiLodIndex < m_TreeLODs.GetCount())
    m_TreeLODs[uiLodIndex].m_State = state;
}

ezKrautLodState ezKrautTreeResource::GetLodState(ezUInt32 uiLodIndex) const
{
  EZ_LOCK(m_LodMutex);

  if (uiLodIndex < m_TreeLODs.GetCount())
    return m_TreeLODs[uiLodIndex].m_State;

  return ezKrautLodState::NotGenerated;
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeResourceDescriptor::Save(ezStreamWriter& inout_stream0) const
{
  ezUInt8 uiVersion = 17;

  inout_stream0 << uiVersion;

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd stream(&inout_stream0, 0, ezCompressedStreamWriterZstd::Compression::Average);
#else
  ezStreamWriter& stream = stream0;
#endif

  inout_stream0 << uiCompressionMode;

  const ezUInt8 uiNumLods = static_cast<ezUInt8>(m_Lods.GetCount());
  stream << uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    const auto& lod = m_Lods[lodIdx];

    stream << static_cast<ezUInt8>(lod.m_LodType);
    stream << lod.m_fMinLodDistance;
    stream << lod.m_fMaxLodDistance;
    stream << lod.m_Vertices.GetCount();
    stream << lod.m_Triangles.GetCount();
    stream << lod.m_SubMeshes.GetCount();

    for (const auto& vtx : lod.m_Vertices)
    {
      stream << vtx.m_vPosition;
      stream << vtx.m_vTexCoord;
      stream << vtx.m_vNormal;
      stream << vtx.m_vTangent;
      stream << vtx.m_fAmbientOcclusion;
      stream << vtx.m_uiColorVariation;
      stream << vtx.m_uiBranchLevel;
      stream << vtx.m_vBendAnchor;
      stream << vtx.m_fAnchorBendStrength;
      stream << vtx.m_fBendAndFlutterStrength;
      stream << vtx.m_uiFlutterPhase;
    }

    for (const auto& tri : lod.m_Triangles)
    {
      stream << tri.m_uiVertexIndex[0];
      stream << tri.m_uiVertexIndex[1];
      stream << tri.m_uiVertexIndex[2];
    }

    for (const auto& sm : lod.m_SubMeshes)
    {
      stream << sm.m_uiFirstTriangle;
      stream << sm.m_uiNumTriangles;
      stream << sm.m_uiMaterialIndex;
    }

    stream << lod.m_uiNumBones;
  }

  const ezUInt8 uiNumMats = static_cast<ezUInt8>(m_Materials.GetCount());
  stream << uiNumMats;

  for (const auto& mat : m_Materials)
  {
    stream << static_cast<ezUInt8>(mat.m_MaterialType);
    stream << static_cast<ezUInt8>(mat.m_BranchType);
    stream << mat.m_sMaterial;
    stream << mat.m_VariationColor;
  }

  stream << m_Details.m_Bounds;
  stream << m_Details.m_vLeafCenter;
  stream << m_Details.m_fStaticColliderRadius;
  stream << m_Details.m_sSurfaceResource;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  stream.FinishCompressedStream().IgnoreResult();

  ezLog::Dev("Compressed Kraut tree data from {0} KB to {1} KB ({2}%%)", ezArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), ezArgF((float)stream.GetCompressedSize() / 1024.0f, 1), ezArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
}

ezResult ezKrautTreeResourceDescriptor::Load(ezStreamReader& inout_stream0)
{
  ezUInt8 uiVersion = 0;

  inout_stream0 >> uiVersion;

  if (uiVersion < 15)
    return EZ_FAILURE;

  if (uiVersion > 17)
  {
    ezLog::Error("Unsupported Kraut tree resource version {0}. Maximum supported version is 17.", uiVersion);
    return EZ_FAILURE;
  }

  ezUInt8 uiCompressionMode = 0;
  inout_stream0 >> uiCompressionMode;

  ezStreamReader* pCompressor = &inout_stream0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream0);
      pCompressor = &decompressorZstd;
      break;
#else
      ezLog::Error("Kraut tree is compressed with zstandard, but support for this compressor is not compiled in.");
      return EZ_FAILURE;
#endif

    default:
      ezLog::Error("Kraut tree is compressed with an unknown algorithm.");
      return EZ_FAILURE;
  }

  ezStreamReader& stream = *pCompressor;

  ezUInt8 uiNumLods = 0;
  stream >> uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    auto& lod = m_Lods.ExpandAndGetRef();

    ezUInt8 lodType;
    stream >> lodType;
    lod.m_LodType = static_cast<ezKrautLodType>(lodType);
    stream >> lod.m_fMinLodDistance;
    stream >> lod.m_fMaxLodDistance;

    ezUInt32 numVertices, numTriangles, numSubMeshes;

    stream >> numVertices;
    stream >> numTriangles;
    stream >> numSubMeshes;

    lod.m_Vertices.SetCountUninitialized(numVertices);
    lod.m_Triangles.SetCountUninitialized(numTriangles);
    lod.m_SubMeshes.SetCount(numSubMeshes); // initialize this one because of the material handle

    for (auto& vtx : lod.m_Vertices)
    {
      stream >> vtx.m_vPosition;
      stream >> vtx.m_vTexCoord;
      stream >> vtx.m_vNormal;
      stream >> vtx.m_vTangent;
      stream >> vtx.m_fAmbientOcclusion;
      stream >> vtx.m_uiColorVariation;
      stream >> vtx.m_uiBranchLevel;
      stream >> vtx.m_vBendAnchor;
      stream >> vtx.m_fAnchorBendStrength;
      stream >> vtx.m_fBendAndFlutterStrength;
      stream >> vtx.m_uiFlutterPhase;
    }

    for (auto& tri : lod.m_Triangles)
    {
      stream >> tri.m_uiVertexIndex[0];
      stream >> tri.m_uiVertexIndex[1];
      stream >> tri.m_uiVertexIndex[2];
    }

    for (auto& sm : lod.m_SubMeshes)
    {
      stream >> sm.m_uiFirstTriangle;
      stream >> sm.m_uiNumTriangles;
      stream >> sm.m_uiMaterialIndex;
    }

    if (uiVersion >= 17)
    {
      stream >> lod.m_uiNumBones;
    }
  }

  ezUInt8 uiNumMats = 0;

  stream >> uiNumMats;
  m_Materials.SetCount(uiNumMats);

  for (auto& mat : m_Materials)
  {
    ezUInt8 matType = 0;
    stream >> matType;
    mat.m_MaterialType = static_cast<ezKrautMaterialType>(matType);

    if (uiVersion >= 16)
    {
      ezUInt8 branchType = 0;
      stream >> branchType;
      mat.m_BranchType = static_cast<ezKrautBranchType>(branchType);
    }

    if (uiVersion >= 14)
    {
      stream >> mat.m_sMaterial;
    }
    else
    {
      ezStringBuilder tmp;
      stream >> tmp;
      stream >> tmp;
    }

    stream >> mat.m_VariationColor;
  }

  stream >> m_Details.m_Bounds;
  stream >> m_Details.m_vLeafCenter;
  stream >> m_Details.m_fStaticColliderRadius;
  stream >> m_Details.m_sSurfaceResource;

  if (uiVersion == 13)
  {
    stream >> uiNumMats;

    for (ezUInt32 i = 0; i < uiNumMats; ++i)
    {
      stream >> m_Materials[i].m_sMaterial;
    }
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(KrautPlugin, KrautPlugin_Resources_KrautTreeResource);
