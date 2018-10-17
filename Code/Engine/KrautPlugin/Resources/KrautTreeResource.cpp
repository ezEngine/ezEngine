#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeResource, 1, ezRTTIDefaultAllocator<ezKrautTreeResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeResource::ezKrautTreeResource()
  : ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire KrautTree
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezKrautTreeResource::UpdateContent(ezStreamReader* Stream)
{
  ezKrautTreeResourceDescriptor desc;
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

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(desc);
}

void ezKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezKrautTreeResource::CreateResource(const ezKrautTreeResourceDescriptor& desc)
{
  m_Descriptor = desc;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeResourceDescriptor::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 2;

  stream << uiVersion;

  const ezUInt8 uiNumLods = m_Lods.GetCount();
  stream << uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    const auto& lod = m_Lods[lodIdx];

    stream << lod.m_Vertices.GetCount();
    stream << lod.m_Triangles.GetCount();
    stream << lod.m_SubMeshes.GetCount();

    for (const auto& vtx : lod.m_Vertices)
    {
      stream << vtx.m_vPosition;
      stream << vtx.m_vTexCoord;
      stream << vtx.m_vNormal;
      stream << vtx.m_vTangent;
      stream << vtx.m_VariationColor;
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
      //stream << sm.m_hMaterial;
    }
  }
}

ezResult ezKrautTreeResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;

  if (uiVersion != 2)
    return EZ_FAILURE;

  ezUInt8 uiNumLods = 0;
  stream >> uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    auto& lod = m_Lods.ExpandAndGetRef();

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
      stream >> vtx.m_VariationColor;
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
      //stream >> sm.m_hMaterial;
    }
  }
  return EZ_SUCCESS;
}
