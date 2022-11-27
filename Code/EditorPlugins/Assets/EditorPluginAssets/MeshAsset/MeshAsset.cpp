#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, 12, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezMat3 CalculateTransformationMatrix(const ezMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return ezBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezMeshAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezTransformStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezMeshAssetProperties* pProp = GetProperties();

  ezMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  if (pProp->m_PrimitiveType == ezMeshPrimitive::File)
  {
    EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc, !transformFlags.IsSet(ezTransformFlags::BackgroundProcessing)));
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
  //const ezMat4 mTrans(mTransformation, ezVec3::ZeroVector());

  ezGeometry::GeoOptions opt;
  opt.m_Transform = ezMat4(mTransformation, ezVec3::ZeroVector());

  auto detail1 = pProp->m_uiDetail;
  auto detail2 = pProp->m_uiDetail2;

  if (pProp->m_PrimitiveType == ezMeshPrimitive::Box)
  {
    geom.AddBox(ezVec3(1.0f), true, opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Capsule)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddCapsule(pProp->m_fRadius, ezMath::Max(0.0f, pProp->m_fHeight), ezMath::Max<ezUInt16>(3, detail1), ezMath::Max<ezUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cone)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, ezMath::Max<ezUInt16>(3, detail1), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Cylinder)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, ezMath::Max<ezUInt16>(3, detail1), opt, ezMath::Clamp(pProp->m_Angle, ezAngle::Degree(0.0f), ezAngle::Degree(360.0f)));
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::GeodesicSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 2;

    geom.AddGeodesicSphere(pProp->m_fRadius, ezMath::Clamp<ezUInt16>(detail1, 0, 6), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::HalfSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddHalfSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, detail1), ezMath::Max<ezUInt16>(1, detail2), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Pyramid)
  {
    geom.AddPyramid(ezVec3(1.0f), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRectXY(ezVec2(1.0f), ezMath::Max<ezUInt16>(1, detail1), ezMath::Max<ezUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, detail1), ezMath::Max<ezUInt16>(2, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Torus)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    float r1 = pProp->m_fRadius;
    float r2 = pProp->m_fRadius2;

    if (r1 == r2)
      r1 = r2 * 0.5f;

    geom.AddTorus(r1, ezMath::Max(r1 + 0.01f, r2), ezMath::Max<ezUInt16>(3, detail1), ezMath::Max<ezUInt16>(3, detail2), true, opt);
  }

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
  mbd.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezMeshTexCoordPrecision::ToResourceFormat(pProp->m_TexCoordPrecision));
  mbd.AddStream(ezGALVertexAttributeSemantic::Normal, ezMeshNormalPrecision::ToResourceFormatNormal(pProp->m_NormalPrecision));
  mbd.AddStream(ezGALVertexAttributeSemantic::Tangent, ezMeshNormalPrecision::ToResourceFormatTangent(pProp->m_NormalPrecision));

  mbd.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);
  desc.AddSubMesh(mbd.GetPrimitiveCount(), 0, 0);
}

ezTransformStatus ezMeshAssetDocument::CreateMeshFromFile(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc, bool bAllowMaterialImport)
{
  ezProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  ezStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return ezStatus("No known importer for this file type.");

  ezModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput = &desc;
  opt.m_MeshNormalsPrecision = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pImporter->Import(opt).Failed())
    return ezStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  bool bSlotCountMissmatch = pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount();
  if (pProp->m_bImportMaterials || bSlotCountMissmatch)
  {
    if (!bAllowMaterialImport && bSlotCountMissmatch)
    {
      return ezTransformStatus(ezTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    ezMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      ezMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  ezMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void ezMeshAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_PrimitiveType != ezMeshPrimitive::File)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_AssetTransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezMeshAssetDocumentGenerator::ezMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
  AddSupportedFileType("vox");
}

ezMeshAssetDocumentGenerator::~ezMeshAssetDocumentGenerator() {}

void ezMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.png";
  }

  {
    ezAssetDocumentGenerator::Info& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "MeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.png";
  }
}

ezStatus ezMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
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
