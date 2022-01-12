#include <EditorPluginPhysX/EditorPluginPhysXPCH.h>

#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocument, 8, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezMat3 CalculateTransformationMatrix(const ezCollisionMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return ezBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

ezCollisionMeshAssetDocument::ezCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh)
  : ezSimpleAssetDocument<ezCollisionMeshAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
  m_bIsConvexMesh = bConvexMesh;
}

void ezCollisionMeshAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
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


ezStatus ezCollisionMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezCollisionMeshAssetProperties* pProp = GetProperties();

  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd compressor(&stream, ezCompressedStreamWriterZstd::Compression::Average);
  ezChunkStreamWriter chunk(compressor);
#else
  ezChunkStreamWriter chunk(stream);
#endif

  stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    range.BeginNextStep("Preparing Mesh");

    ezPhysXCookingMesh xMesh;

    if (!m_bIsConvexMesh || pProp->m_ConvexMeshType == ezConvexCollisionMeshType::ConvexHull || pProp->m_ConvexMeshType == ezConvexCollisionMeshType::ConvexDecomposition)
    {
      EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(xMesh));
    }
    else
    {
      const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

      xMesh.m_bFlipNormals = ezGraphicsUtils::IsTriangleFlipRequired(mTransformation);

      ezGeometry geom;
      ezGeometry::GeoOptions opt;
      opt.m_Transform = ezMat4(mTransformation, ezVec3::ZeroVector());

      if (pProp->m_ConvexMeshType == ezConvexCollisionMeshType::Cylinder)
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

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromFile(ezPhysXCookingMesh& outMesh)
{
  ezCollisionMeshAssetProperties* pProp = GetProperties();

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

  if (pImporter->Import(opt).Failed())
    return ezStatus("Model importer was unable to read this asset.");

  const auto& meshBuffer = meshDesc.MeshBufferDesc();

  const ezUInt32 uiNumTriangles = meshBuffer.GetPrimitiveCount();
  const ezUInt32 uiNumVertices = meshBuffer.GetVertexCount();

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
  {
    pProp->m_Slots.SetCount(meshDesc.GetSubMeshes().GetCount());

    for (ezUInt32 matIdx = 0; matIdx < pImporter->m_OutputMaterials.GetCount(); ++matIdx)
    {
      const ezInt32 subMeshIdx = pImporter->m_OutputMaterials[matIdx].m_iReferencedByMesh;
      if (subMeshIdx < 0)
        continue;

      pProp->m_Slots[subMeshIdx].m_sLabel = pImporter->m_OutputMaterials[matIdx].m_sName;

      const auto subMeshInfo = meshDesc.GetSubMeshes()[subMeshIdx];

      // update the triangle material information
      for (ezUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
      {
        outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = subMeshIdx;
      }
    }

    ApplyNativePropertyChangesToObjectManager();
    pProp = GetProperties();
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromGeom(ezGeometry& geom, ezPhysXCookingMesh& outMesh)
{
  ezCollisionMeshAssetProperties* pProp = GetProperties();

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

ezStatus ezCollisionMeshAssetDocument::WriteToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh, const ezCollisionMeshAssetProperties* pProp)
{
  ezHybridArray<ezString, 32> surfaces;

  for (const auto& slot : pProp->m_Slots)
  {
    surfaces.PushBack(slot.m_sResource);
  }

  ezPhysXCooking::MeshType meshType = ezPhysXCooking::MeshType::Triangle;

  if (pProp->m_bIsConvexMesh)
  {
    if (pProp->m_ConvexMeshType == ezConvexCollisionMeshType::ConvexDecomposition)
    {
      meshType = ezPhysXCooking::MeshType::ConvexDecomposition;
    }
    else
    {
      meshType = ezPhysXCooking::MeshType::ConvexHull;
    }
  }

  return ezPhysXCooking::WriteResourceToStream(stream, mesh, surfaces, meshType, pProp->m_uiMaxConvexPieces);
}

ezStatus ezCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void ezCollisionMeshAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ConvexMeshType != ezConvexCollisionMeshType::ConvexHull)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_AssetTransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollisionMeshAssetDocumentGenerator::ezCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezCollisionMeshAssetDocumentGenerator::~ezCollisionMeshAssetDocumentGenerator() = default;

void ezCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("ezCollisionMeshAsset");

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "CollisionMeshImport.TriangleMesh";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
  }
}

ezStatus ezCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezCollisionMeshAssetDocument* pAssetDoc = ezDynamicCast<ezCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConvexCollisionMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezConvexCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezConvexCollisionMeshAssetDocumentGenerator::ezConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezConvexCollisionMeshAssetDocumentGenerator::~ezConvexCollisionMeshAssetDocumentGenerator() = default;

void ezConvexCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("ezConvexCollisionMeshAsset");

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "CollisionMeshImport.ConvexMesh";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Collision_Mesh_Convex.png";
  }
}

ezStatus ezConvexCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezCollisionMeshAssetDocument* pAssetDoc = ezDynamicCast<ezCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
