#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltCooking/JoltCooking.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshAssetDocument, 10, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezMat3 CalculateTransformationMatrix(const ezJoltCollisionMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = ezMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = ezMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = ezMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  const ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  return ezBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, us);
}

ezJoltCollisionMeshAssetDocument::ezJoltCollisionMeshAssetDocument(ezStringView sDocumentPath, bool bConvexMesh)
  : ezSimpleAssetDocument<ezJoltCollisionMeshAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple)
{
  m_bIsConvexMesh = bConvexMesh;
}

void ezJoltCollisionMeshAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // this logic is for backwards compatibility, to sync the convex state with existing data
  if (m_bIsConvexMesh)
  {
    GetPropertyObject()->GetTypeAccessor().SetValue("IsConvexMesh", m_bIsConvexMesh);
  }
  else
  {
    m_bIsConvexMesh = GetPropertyObject()->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  }

  // the GetProperties object seems distinct from the GetPropertyObject, so keep them in sync
  GetProperties()->m_bIsConvexMesh = m_bIsConvexMesh;
}


//////////////////////////////////////////////////////////////////////////


ezTransformStatus ezJoltCollisionMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezJoltCollisionMeshAssetProperties* pProp = GetProperties();

  EZ_ASSERT_DEV(AssetHeader.GetFileVersion() == 10, "Version change");
  // Please check that the code here is in sync with ezSceneExportModifier_JoltStaticMeshConversion::ModifyWorld()

  const ezUInt8 uiVersion = 3; // read in ezJoltMeshResource::UpdateContent()
  stream << uiVersion;

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd compressor(&stream, 0, ezCompressedStreamWriterZstd::Compression::Average);
  ezChunkStreamWriter chunk(compressor);
#else
  ezChunkStreamWriter chunk(stream);
#endif

  stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    range.BeginNextStep("Preparing Mesh");

    ezJoltCookingMesh xMesh;

    if (!m_bIsConvexMesh || pProp->m_ConvexMeshType == ezJoltConvexCollisionMeshType::ConvexHull || pProp->m_ConvexMeshType == ezJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(xMesh));
    }
    else
    {
      const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

      xMesh.m_bFlipNormals = ezGraphicsUtils::IsTriangleFlipRequired(mTransformation);

      ezGeometry geom;
      ezGeometry::GeoOptions opt;
      opt.m_Transform = ezMat4(mTransformation, ezVec3::MakeZero());

      if (pProp->m_ConvexMeshType == ezJoltConvexCollisionMeshType::Cylinder)
      {
        geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, ezMath::Clamp<ezUInt16>(pProp->m_uiDetail, 3, 32), opt);
      }

      EZ_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, xMesh));
    }

    range.BeginNextStep("Writing Result");
    EZ_SUCCEED_OR_RETURN(WriteToStream(chunk, xMesh, GetProperties()));
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  EZ_SUCCEED_OR_RETURN(compressor.FinishCompressedStream());

  ezLog::Dev("Compressed collision mesh data from {0} KB to {1} KB ({2}%%)", ezArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), ezArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), ezArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezJoltCollisionMeshAssetDocument::CreateMeshFromFile(ezJoltCookingMesh& outMesh)
{
  ezJoltCollisionMeshAssetProperties* pProp = GetProperties();

  ezStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return ezStatus("No known importer for this file type.");

  ezMeshResourceDescriptor meshDesc;

  ezModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_pMeshOutput = &meshDesc;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  // include tags
  {
    ezHybridArray<ezStringView, 8> tags;
    pProp->m_sMeshIncludeTags.Split(false, tags, ";");
    for (ezStringView tag : tags)
    {
      tag.Trim();
      opt.m_MeshIncludeTags.PushBack(tag);
    }
  }

  // exclude tags
  {
    ezHybridArray<ezStringView, 8> tags;
    pProp->m_sMeshExcludeTags.Split(false, tags, ";");
    for (ezStringView tag : tags)
    {
      tag.Trim();
      opt.m_MeshExcludeTags.PushBack(tag);
    }
  }

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError = pProp->m_uiMaxSimplificationError;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

  if (pImporter->Import(opt).Failed())
    return ezStatus("Model importer was unable to read this asset.");

  const auto& meshBuffer = meshDesc.MeshBufferDesc();

  const ezUInt32 uiNumTriangles = meshBuffer.GetPrimitiveCount();
  const ezUInt32 uiNumVertices = meshBuffer.GetVertexCount();

  if (uiNumTriangles < 3 || uiNumVertices < 3)
    return ezStatus("Invalid collision mesh.");

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(uiNumTriangles);
  outMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);

  for (ezUInt32 uiTriangle = 0; uiTriangle < uiNumTriangles; ++uiTriangle)
  {
    outMesh.m_PolygonSurfaceID[uiTriangle] = 0;  // default value, will be updated below when extracting materials.
    outMesh.m_VerticesInPolygon[uiTriangle] = 3; // Triangles!
  }

  // Extract vertices
  {
    const ezUInt8* pVertexData = meshDesc.MeshBufferDesc().GetVertexData(0, 0).GetPtr();
    const ezUInt32 uiVertexSize = meshBuffer.GetVertexDataSize();

    outMesh.m_Vertices.SetCountUninitialized(uiNumVertices);
    for (ezUInt32 v = 0; v < uiNumVertices; ++v)
    {
      outMesh.m_Vertices[v] = *reinterpret_cast<const ezVec3*>(pVertexData + v * uiVertexSize);
    }
  }

  // Extract indices
  {
    outMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);

    if (meshBuffer.Uses32BitIndices())
    {
      const ezUInt32* pIndices = reinterpret_cast<const ezUInt32*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (ezUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
    else
    {
      const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (ezUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
  }

  // Extract Material Information
  if (m_bIsConvexMesh)
  {
    meshDesc.CollapseSubMeshes();
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Convex";
    pProp->m_Slots[0].m_sResource = pProp->m_sConvexMeshSurface;

    const auto subMeshInfo = meshDesc.GetSubMeshes()[0];

    for (ezUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
    {
      outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0;
    }
  }
  else
  {
    pProp->m_Slots.SetCount(meshDesc.GetSubMeshes().GetCount());

    for (ezUInt32 matIdx = 0; matIdx < pImporter->m_OutputMaterials.GetCount(); ++matIdx)
    {
      const ezInt32 subMeshIdx = pImporter->m_OutputMaterials[matIdx].m_iReferencedByMesh;
      if (subMeshIdx < 0)
        continue;

      pProp->m_Slots[subMeshIdx].m_sLabel = pImporter->m_OutputMaterials[matIdx].m_sName;

      const auto subMeshInfo = meshDesc.GetSubMeshes()[subMeshIdx];

      if (pProp->m_Slots[subMeshIdx].m_bExclude)
      {
        // update the triangle material information
        for (ezUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0xFFFF;
        }
      }
      else
      {
        // update the triangle material information
        for (ezUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = subMeshIdx;
        }
      }
    }

    ApplyNativePropertyChangesToObjectManager();
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezJoltCollisionMeshAssetDocument::CreateMeshFromGeom(ezGeometry& geom, ezJoltCookingMesh& outMesh)
{
  ezJoltCollisionMeshAssetProperties* pProp = GetProperties();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
      EZ_IGNORE_UNUSED(pProp);
    }
  }

  // copy vertex positions
  {
    outMesh.m_Vertices.SetCountUninitialized(geom.GetVertices().GetCount());
    for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      outMesh.m_Vertices[v] = geom.GetVertices()[v].m_vPosition;
    }
  }

  // Copy Polygon Data
  {
    outMesh.m_PolygonSurfaceID.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_VerticesInPolygon.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_PolygonIndices.Reserve(geom.GetPolygons().GetCount() * 4);

    for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      const auto& poly = geom.GetPolygons()[p];
      outMesh.m_VerticesInPolygon[p] = poly.m_Vertices.GetCount();
      outMesh.m_PolygonSurfaceID[p] = 0;

      for (ezUInt32 posIdx : poly.m_Vertices)
      {
        outMesh.m_PolygonIndices.PushBack(posIdx);
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezJoltCollisionMeshAssetDocument::WriteToStream(ezChunkStreamWriter& inout_stream, const ezJoltCookingMesh& mesh, const ezJoltCollisionMeshAssetProperties* pProp)
{
  ezHybridArray<ezString, 32> surfaces;

  for (const auto& slot : pProp->m_Slots)
  {
    surfaces.PushBack(slot.m_sResource);
  }

  ezJoltCooking::MeshType meshType = ezJoltCooking::MeshType::Triangle;

  if (pProp->m_bIsConvexMesh)
  {
    if (pProp->m_ConvexMeshType == ezJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      meshType = ezJoltCooking::MeshType::ConvexDecomposition;
    }
    else
    {
      meshType = ezJoltCooking::MeshType::ConvexHull;
    }
  }

  return ezJoltCooking::WriteResourceToStream(inout_stream, mesh, surfaces, meshType, pProp->m_uiMaxConvexPieces);
}

ezTransformStatus ezJoltCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void ezJoltCollisionMeshAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ConvexMeshType != ezJoltConvexCollisionMeshType::ConvexHull)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_TransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezJoltCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezJoltCollisionMeshAssetDocumentGenerator::ezJoltCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezJoltCollisionMeshAssetDocumentGenerator::~ezJoltCollisionMeshAssetDocumentGenerator() = default;

void ezJoltCollisionMeshAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "Jolt_Colmesh_Triangle";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  }
}

ezStatus ezJoltCollisionMeshAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping collision mesh import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezJoltCollisionMeshAssetDocument* pAssetDoc = ezDynamicCast<ezJoltCollisionMeshAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  ezLog::Success("Imported collision mesh: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltConvexCollisionMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezJoltConvexCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezJoltConvexCollisionMeshAssetDocumentGenerator::ezJoltConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezJoltConvexCollisionMeshAssetDocumentGenerator::~ezJoltConvexCollisionMeshAssetDocumentGenerator() = default;

void ezJoltConvexCollisionMeshAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "Jolt_Colmesh_Convex";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  }
}

ezStatus ezJoltConvexCollisionMeshAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping convex collision mesh import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezJoltCollisionMeshAssetDocument* pAssetDoc = ezDynamicCast<ezJoltCollisionMeshAssetDocument*>(pDoc);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  ezLog::Success("Imported convex collision mesh: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}
