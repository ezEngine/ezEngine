#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Surfaces/SurfaceResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Textures/Texture2DResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeResource, 1, ezRTTIDefaultAllocator<ezKrautTreeResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeResource::ezKrautTreeResource()
    : ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
  m_Details.m_Bounds.SetInvalid();
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
  m_TreeLODs.Clear();
  m_Details = desc.m_Details;

  ezHybridArray<ezMaterialResourceHandle, 16> allMaterials;

  ezStringBuilder sMatName;

  for (const auto& mat : desc.m_Materials)
  {
    ezUInt32 uiTexHash = static_cast<ezUInt32>(mat.m_MaterialType);
    uiTexHash = ezHashing::xxHash32(&mat.m_VariationColor, sizeof(mat.m_VariationColor), uiTexHash);
    uiTexHash = ezHashing::xxHash32(mat.m_sDiffuseTexture.GetData(), mat.m_sDiffuseTexture.GetElementCount(), uiTexHash);
    uiTexHash = ezHashing::xxHash32(mat.m_sNormalMapTexture.GetData(), mat.m_sNormalMapTexture.GetElementCount(), uiTexHash);

    sMatName.Format("KrautMaterial_{0}", uiTexHash);

    auto hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMatName);

    if (!hMaterial.IsValid())
    {
      ezMaterialResourceDescriptor md;

      switch (mat.m_MaterialType)
      {
        case ezKrautMaterialType::Branch:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Branch.ezMaterial");
          break;

        case ezKrautMaterialType::Frond:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Frond.ezMaterial");
          break;

        case ezKrautMaterialType::Leaf:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Leaf.ezMaterial");
          break;

        case ezKrautMaterialType::StaticImpostor:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/StaticImpostor.ezMaterial");
          break;

        case ezKrautMaterialType::BillboardImpostor:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/BillboardImpostor.ezMaterial");
          break;
      }

      auto& m1 = md.m_Texture2DBindings.ExpandAndGetRef();
      m1.m_Name.Assign("BaseTexture");
      m1.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(mat.m_sDiffuseTexture);

      auto& m2 = md.m_Texture2DBindings.ExpandAndGetRef();
      m2.m_Name.Assign("NormalTexture");
      m2.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(mat.m_sNormalMapTexture);

      auto& p1 = md.m_Parameters.ExpandAndGetRef();
      p1.m_Name.Assign("BaseColor");
      p1.m_Value = mat.m_VariationColor;

      hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(sMatName, md, mat.m_sDiffuseTexture);
    }

    allMaterials.PushBack(hMaterial);
  }

  ezStringBuilder sResName, sResDesc;

  for (ezUInt32 lodIdx = 0; lodIdx < desc.m_Lods.GetCount(); ++lodIdx)
  {
    const auto& lodSrc = desc.m_Lods[lodIdx];
    auto& lodDst = m_TreeLODs.ExpandAndGetRef();

    lodDst.m_LodType = lodSrc.m_LodType;
    lodDst.m_fMinLodDistance = lodSrc.m_fMinLodDistance;
    lodDst.m_fMaxLodDistance = lodSrc.m_fMaxLodDistance;

    ezMeshResourceDescriptor md;
    auto& buffer = md.MeshBufferDesc();

    const ezUInt32 uiNumVertices = lodSrc.m_Vertices.GetCount();
    const ezUInt32 uiNumTriangles = lodSrc.m_Triangles.GetCount();
    const ezUInt32 uiSubMeshes = lodSrc.m_SubMeshes.GetCount();

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
    buffer.AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (ezUInt32 v = 0; v < uiNumVertices; ++v)
    {
      const auto& vtx = lodSrc.m_Vertices[v];

      buffer.SetVertexData<ezVec3>(0, v, vtx.m_vPosition);
      buffer.SetVertexData<ezVec2>(1, v, ezVec2(vtx.m_vTexCoord.x, vtx.m_vTexCoord.y));
      buffer.SetVertexData<ezVec2>(2, v, ezVec2(vtx.m_vTexCoord.z, vtx.m_fAmbientOcclusion));
      buffer.SetVertexData<ezVec3>(3, v, vtx.m_vNormal);
      buffer.SetVertexData<ezVec3>(4, v, vtx.m_vTangent);
      buffer.SetVertexData<ezColorGammaUB>(5, v, vtx.m_VariationColor);
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

    for (ezUInt32 mat = 0; mat < desc.m_Materials.GetCount(); ++mat)
    {
      md.SetMaterial(mat, allMaterials[mat].GetResourceID());
    }

    sResName.Format("{0}_{1}_LOD{2}", GetResourceID(), GetCurrentResourceChangeCounter(), lodIdx);
    sResDesc.Format("{0}_{1}_LOD{2}", GetResourceDescription(), GetCurrentResourceChangeCounter(), lodIdx);

    lodDst.m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResName, md, sResDesc);
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeResourceDescriptor::Save(ezStreamWriter& stream0) const
{
  ezUInt8 uiVersion = 12;

  stream0 << uiVersion;

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd stream(&stream0, ezCompressedStreamWriterZstd::Compression::Average);
#else
  ezStreamWriter& stream = stream0;
#endif

  stream0 << uiCompressionMode;

  const ezUInt8 uiNumLods = m_Lods.GetCount();
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
      stream << vtx.m_VariationColor;
      stream << vtx.m_fAmbientOcclusion;
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
  }

  const ezUInt8 uiNumMats = m_Materials.GetCount();
  stream << uiNumMats;

  for (const auto& mat : m_Materials)
  {
    stream << static_cast<ezUInt8>(mat.m_MaterialType);
    stream << mat.m_sDiffuseTexture;
    stream << mat.m_sNormalMapTexture;
    stream << mat.m_VariationColor;
  }

  stream << m_Details.m_Bounds;
  stream << m_Details.m_vLeafCenter;
  stream << m_Details.m_fStaticColliderRadius;
  stream << m_Details.m_sSurfaceResource;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  stream.FinishCompressedStream();

  ezLog::Dev("Compressed Kraut tree data from {0} KB to {1} KB ({2}%%)", ezArgF((float)stream.GetUncompressedSize() / 1024.0f, 1),
             ezArgF((float)stream.GetCompressedSize() / 1024.0f, 1),
             ezArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
}

ezResult ezKrautTreeResourceDescriptor::Load(ezStreamReader& stream0)
{
  ezUInt8 uiVersion = 0;

  stream0 >> uiVersion;

  if (uiVersion != 12)
    return EZ_FAILURE;

  ezUInt8 uiCompressionMode = 0;
  stream0 >> uiCompressionMode;

  ezStreamReader* pCompressor = &stream0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&stream0);
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
      stream >> vtx.m_VariationColor;
      stream >> vtx.m_fAmbientOcclusion;
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
  }

  ezUInt8 uiNumMats = 0;
  ;
  stream >> uiNumMats;
  m_Materials.SetCount(uiNumMats);

  for (auto& mat : m_Materials)
  {
    ezUInt8 matType = 0;
    stream >> matType;
    mat.m_MaterialType = static_cast<ezKrautMaterialType>(matType);
    stream >> mat.m_sDiffuseTexture;
    stream >> mat.m_sNormalMapTexture;
    stream >> mat.m_VariationColor;
  }

  stream >> m_Details.m_Bounds;
  stream >> m_Details.m_vLeafCenter;
  stream >> m_Details.m_fStaticColliderRadius;
  stream >> m_Details.m_sSurfaceResource;

  return EZ_SUCCESS;
}
