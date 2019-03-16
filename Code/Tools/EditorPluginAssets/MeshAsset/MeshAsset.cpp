#include <EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, 8, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

static ezMat3 CalculateTransformationMatrix(const ezMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);
  const float sx = ezMath::Clamp(pProp->m_vNonUniformScaling.x, 0.0001f, 10000.0f);
  const float sy = ezMath::Clamp(pProp->m_vNonUniformScaling.y, 0.0001f, 10000.0f);
  const float sz = ezMath::Clamp(pProp->m_vNonUniformScaling.z, 0.0001f, 10000.0f);

  return ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, us, sx, sy, sz);
}

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezMeshAssetProperties>(szDocumentPath, true)
{
}

ezStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
                                                     const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
                                                     bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezMeshAssetProperties* pProp = GetProperties();

  ezMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Mesh");

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {
    EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));
  }
  else
  {
    CreateMeshFromGeom(pProp, desc);
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezMeshAssetDocument::CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc)
{
  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

  ezGeometry geom;
  const ezMat4 mTrans(mTransformation, ezVec3::ZeroVector());

  if (pProp->m_PrimitiveType == ezMeshPrimitive::Box)
  {
    geom.AddTexturedBox(ezVec3(1.0f), ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Capsule)
  {
    geom.AddCapsule(pProp->m_fRadius, ezMath::Max(0.0f, pProp->m_fHeight), ezMath::Max<ezUInt16>(3, pProp->m_uiDetail),
                    ezMath::Max<ezUInt16>(1, pProp->m_uiDetail2), ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cone)
  {
    geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cylinder)
  {
    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2,
                     ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezColor::White, mTrans, 0,
                     ezMath::Clamp(pProp->m_Angle, ezAngle::Degree(0.0f), ezAngle::Degree(360.0f)));
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::GeodesicSphere)
  {
    geom.AddGeodesicSphere(pProp->m_fRadius, ezMath::Clamp<ezUInt16>(pProp->m_uiDetail, 0, 6), ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::HalfSphere)
  {
    geom.AddHalfSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(1, pProp->m_uiDetail2),
                       pProp->m_bCap, ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Pyramid)
  {
    geom.AddPyramid(ezVec3(1.0f), pProp->m_bCap, ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Rect)
  {
    ezMat4 mTrans2  = mTrans;

    mTrans2.Element(2, 0) = -mTrans.Element(2, 0);
    mTrans2.Element(2, 1) = -mTrans.Element(2, 1);
    mTrans2.Element(2, 2) = -mTrans.Element(2, 2);

    geom.AddTesselatedRectXY(ezVec2(1.0f), ezColor::White, ezMath::Max<ezUInt16>(1, pProp->m_uiDetail), ezMath::Max<ezUInt16>(1, pProp->m_uiDetail2), mTrans2);
    //geom.AddRectXY(ezVec2(1.0f), ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Sphere)
  {
    geom.AddSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, pProp->m_uiDetail), ezMath::Max<ezUInt16>(2, pProp->m_uiDetail2),
                   ezColor::White, mTrans);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Torus)
  {
    geom.AddTorus(pProp->m_fRadius, ezMath::Max(pProp->m_fRadius + 0.01f, pProp->m_fRadius2), ezMath::Max<ezUInt16>(3, pProp->m_uiDetail),
                  ezMath::Max<ezUInt16>(3, pProp->m_uiDetail2), ezColor::White, mTrans);
  }

  geom.ComputeFaceNormals();
  geom.TriangulatePolygons(4);
  geom.ComputeTangents();

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

    // Set material for mesh.
    if (!pProp->m_Slots.IsEmpty())
      desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
    else
      desc.SetMaterial(0, "");
  }

  auto& mbd = desc.MeshBufferDesc();
  mbd.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  mbd.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  mbd.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  mbd.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);

  mbd.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);
  desc.AddSubMesh(mbd.GetPrimitiveCount(), 0, 0);
}

ezStatus ezMeshAssetDocument::CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc)
{
  EZ_PROFILE_SCOPE("CreateMeshFromFile");
  ezProgressRange range("Mesh Import", 6, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

  ezSharedPtr<ezModelImporter::Scene> pScene;
  ezModelImporter::Mesh* pMesh = nullptr;
  EZ_SUCCEED_OR_RETURN(ezMeshImportUtils::TryImportMesh(pScene, pMesh, pProp->m_sMeshFile, pProp->m_sSubMeshName, mTransformation,
                                                        pProp->m_bRecalculateNormals, pProp->m_bInvertNormals, range, desc, false));

  range.BeginNextStep("Importing Materials");

  // Optional material slot count correction & material import.
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != pMesh->GetNumSubMeshes())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

    ezMeshImportUtils::UpdateMaterialSlots(GetDocumentPath(), *pScene, *pMesh, pProp->m_bImportMaterials,
                                           pProp->m_bUseSubFolderForImportedMaterials, pProp->m_sMeshFile, pProp->m_Slots);

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  range.BeginNextStep("Setting Materials");

  ezMeshImportUtils::AddMeshToDescriptor(desc, *pScene, *pMesh, pProp->m_Slots);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezMeshAssetDocumentGenerator::ezMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("ply");
}

ezMeshAssetDocumentGenerator::~ezMeshAssetDocumentGenerator() {}

void ezMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                  ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.png";
  }

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "MeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.png";
  }
}

ezStatus ezMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezMeshAssetDocument* pAssetDoc = ezDynamicCast<ezMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  if (info.m_sName == "MeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "MeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return ezStatus(EZ_SUCCESS);
}
