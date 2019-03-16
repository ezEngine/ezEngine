#include <EditorPluginPhysXPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Material.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/VertexData.h>
#include <PhysXCooking/PhysXCooking.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocument, 6, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

static ezMat3 CalculateTransformationMatrix(const ezCollisionMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);
  const float sx = ezMath::Clamp(pProp->m_vNonUniformScaling.x, 0.0001f, 10000.0f);
  const float sy = ezMath::Clamp(pProp->m_vNonUniformScaling.y, 0.0001f, 10000.0f);
  const float sz = ezMath::Clamp(pProp->m_vNonUniformScaling.z, 0.0001f, 10000.0f);

  return ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, us, sx, sy, sz);
}

ezCollisionMeshAssetDocument::ezCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh)
    : ezSimpleAssetDocument<ezCollisionMeshAssetProperties>(szDocumentPath, true)
{
  m_bIsConvexMesh = bConvexMesh;
}

void ezCollisionMeshAssetDocument::InitializeAfterLoading()
{
  SUPER::InitializeAfterLoading();

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

const char* ezCollisionMeshAssetDocument::QueryAssetType() const
{
  if (m_bIsConvexMesh)
    return "Collision Mesh (Convex)";

  return "Collision Mesh";
}



//////////////////////////////////////////////////////////////////////////


ezStatus ezCollisionMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
                                                              const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
                                                              bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezCollisionMeshAssetProperties* pProp = GetProperties();

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);
  const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(mTransformation);

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

    // TODO verify
    xMesh.m_bFlipNormals = (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);

    if (!m_bIsConvexMesh || pProp->m_ConvexMeshType == ezConvexCollisionMeshType::ConvexHull)
    {
      EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(mTransformation, xMesh));
    }
    else
    {
      ezGeometry geom;
      const ezMat4 mTrans(mTransformation, ezVec3::ZeroVector());

      if (pProp->m_ConvexMeshType == ezConvexCollisionMeshType::Cylinder)
      {
        geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f,
                                 ezMath::Clamp<ezUInt16>(pProp->m_uiDetail, 3, 32), ezColor::White, mTrans);
      }

      EZ_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, xMesh));
    }

    range.BeginNextStep("Writing Result");
    EZ_SUCCEED_OR_RETURN(WriteToStream(chunk, xMesh, GetProperties()));
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream();

  ezLog::Dev("Compressed collision mesh data from {0} KB to {1} KB ({2}%%)", ezArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1),
             ezArgF((float)compressor.GetCompressedSize() / 1024.0f, 1),
             ezArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return ezStatus(EZ_SUCCESS);
}

static ezStatus ImportMesh(const char* filename, const char* subMeshFilename, ezSharedPtr<ezModelImporter::Scene>& outScene,
                           ezModelImporter::Mesh*& outMesh, ezString& outMeshFileAbs)
{
  ezStopwatch timer;

  outMeshFileAbs = filename;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(outMeshFileAbs))
  {
    return ezStatus(ezFmt("Could not make path absolute: '{0};", outMeshFileAbs));
  }

  EZ_SUCCEED_OR_RETURN(ezModelImporter::Importer::GetSingleton()->ImportMesh(outMeshFileAbs, subMeshFilename, false, outScene, outMesh));

  ezLog::Debug("Mesh Import time: {0}s", ezArgF(timer.GetRunningTotal().GetSeconds(), 2));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezCollisionMeshAssetDocument::CreateMeshFromFile(const ezMat3& mTransformation, ezPhysXCookingMesh& outMesh)
{
  using namespace ezModelImporter;

  ezCollisionMeshAssetProperties* pProp = GetProperties();

  ezSharedPtr<Scene> scene;
  Mesh* mesh = nullptr;
  ezString sMeshFileAbs;
  EZ_SUCCEED_OR_RETURN(ImportMesh(pProp->m_sMeshFile, pProp->m_sSubMeshName, scene, mesh, sMeshFileAbs));

  const ezModelImporter::TypedVertexDataStreamView<ezVec3> positionStream(*mesh->GetDataStream(ezGALVertexAttributeSemantic::Position));

  const ezArrayPtr<Mesh::Triangle> triangles = mesh->GetTriangles();

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(triangles.GetCount());
  outMesh.m_VerticesInPolygon.SetCountUninitialized(triangles.GetCount());
  for (ezUInt32 uiTriangle = 0; uiTriangle < triangles.GetCount(); ++uiTriangle)
  {
    outMesh.m_PolygonSurfaceID[uiTriangle] = 0;  // default value, will be updated below when extracting materials.
    outMesh.m_VerticesInPolygon[uiTriangle] = 3; // Triangles!
  }

  // Extract vertices and indices
  {
    const ezGALVertexAttributeSemantic::Enum streamSemantics[] = {ezGALVertexAttributeSemantic::Position};
    ezHashTable<Mesh::DataIndexBundle<1>, ezUInt32> dataIndicesToVertexIndices;
    mesh->GenerateInterleavedVertexMapping(streamSemantics, dataIndicesToVertexIndices, outMesh.m_PolygonIndices);

    // outMesh.m_PolygonIndices is now ready and we have a mapping from mesh data index to vertex index. Remains only to copy over the
    // vertices to their correct places.

    outMesh.m_Vertices.SetCountUninitialized(dataIndicesToVertexIndices.GetCount());
    for (auto it = dataIndicesToVertexIndices.GetIterator(); it.IsValid(); ++it)
    {
      ezVec3 vPosition = positionStream.GetValue(it.Key()[0]);
      outMesh.m_Vertices[it.Value()] = mTransformation.TransformDirection(vPosition);
    }
  }

  // Extract Material Information
  {
    ezStringBuilder sMatName;

    pProp->m_Slots.SetCount(mesh->GetNumSubMeshes());

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < mesh->GetNumSubMeshes(); ++subMeshIdx)
    {
      const ezModelImporter::SubMesh& subMesh = mesh->GetSubMesh(subMeshIdx);
      const ezModelImporter::Material* material = scene->GetMaterial(subMesh.m_Material);

      // update the triangle material information
      for (ezUInt32 tri = 0; tri < subMesh.m_uiTriangleCount; ++tri)
      {
        outMesh.m_PolygonSurfaceID[subMesh.m_uiFirstTriangle + tri] = subMeshIdx;
      }

      pProp->m_Slots[subMeshIdx].m_sLabel = material->m_Name;
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

ezStatus ezCollisionMeshAssetDocument::WriteToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh,
                                                     const ezCollisionMeshAssetProperties* pProp)
{
  ezHybridArray<ezString, 32> surfaces;

  for (const auto& slot : pProp->m_Slots)
  {
    surfaces.PushBack(slot.m_sResource);
  }

  return ezPhysXCooking::WriteResourceToStream(stream, mesh, surfaces, pProp->m_bIsConvexMesh);
}

ezStatus ezCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollisionMeshAssetDocumentGenerator::ezCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("ply");
}

ezCollisionMeshAssetDocumentGenerator::~ezCollisionMeshAssetDocumentGenerator() = default;

void ezCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                           ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
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

ezStatus ezCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                         ezDocument*& out_pGeneratedDocument)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConvexCollisionMeshAssetDocumentGenerator, 1,
                                ezRTTIDefaultAllocator<ezConvexCollisionMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezConvexCollisionMeshAssetDocumentGenerator::ezConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("ply");
}

ezConvexCollisionMeshAssetDocumentGenerator::~ezConvexCollisionMeshAssetDocumentGenerator() = default;

void ezConvexCollisionMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                                 ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
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

ezStatus ezConvexCollisionMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath,
                                                               const ezAssetDocumentGenerator::Info& info,
                                                               ezDocument*& out_pGeneratedDocument)
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
