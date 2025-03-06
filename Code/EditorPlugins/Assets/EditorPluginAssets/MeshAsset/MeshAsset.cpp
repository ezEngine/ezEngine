#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, 13, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezMat3 CalculateTransformationMatrix(const ezMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = ezMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = ezMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = ezMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  const ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  return ezBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, us);
}

ezMeshAssetDocument::ezMeshAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezMeshAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezTransformStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
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

  // if there is no material set for a slot, use the "Pattern" material as a fallback
  for (ezUInt32 matIdx = 0; matIdx < desc.GetMaterials().GetCount(); ++matIdx)
  {
    if (desc.GetMaterials()[matIdx].m_sPath.IsEmpty())
    {
      // Data/Base/Materials/Common/Pattern.ezMaterialAsset
      desc.SetMaterial(matIdx, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }");
    }
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezMeshAssetDocument::CreateMeshFromGeom(ezMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc)
{
  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

  ezGeometry geom;
  // const ezMat4 mTrans(mTransformation, ezVec3::MakeZero());

  ezGeometry::GeoOptions opt;
  opt.m_Transform = ezMat4(mTransformation, ezVec3::MakeZero());

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

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, ezMath::Max<ezUInt16>(3, detail1), opt, ezMath::Clamp(pProp->m_Angle, ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(360.0f)));
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
    geom.AddPyramid(1.0f, 1.0f, pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRect(ezVec2(1.0f), ezMath::Max<ezUInt16>(1, detail1), ezMath::Max<ezUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == ezMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddStackedSphere(pProp->m_fRadius, ezMath::Max<ezUInt16>(3, detail1), ezMath::Max<ezUInt16>(2, detail2), opt);
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

  // the the procedurally generated geometry we can always use fixed, low precision data, because we know that the geometry isn't detailed enough to run into problems
  // and then we can unclutter the UI a little by not showing those options at all
  auto& mbd = desc.MeshBufferDesc();
  mbd.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  mbd.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezMeshTexCoordPrecision::ToResourceFormat(ezMeshTexCoordPrecision::_16Bit /*pProp->m_TexCoordPrecision*/));
  mbd.AddStream(ezGALVertexAttributeSemantic::Normal, ezMeshNormalPrecision::ToResourceFormatNormal(ezMeshNormalPrecision::_10Bit /*pProp->m_NormalPrecision*/));
  mbd.AddStream(ezGALVertexAttributeSemantic::Tangent, ezMeshNormalPrecision::ToResourceFormatTangent(ezMeshNormalPrecision::_10Bit /*pProp->m_NormalPrecision*/));

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
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError = pProp->m_uiMaxSimplificationError;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

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
    pInfo->m_TransformDependencies.Remove(sMeshFile);
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

ezMeshAssetDocumentGenerator::~ezMeshAssetDocumentGenerator() = default;

void ezMeshAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport.WithMaterials";
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }

  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "MeshImport.NoMaterials";
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }
}

ezStatus ezMeshAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  ezOSFile::FindFreeFilename(sOutFile);

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezMeshAssetDocument* pAssetDoc = ezDynamicCast<ezMeshAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  if (sMode == "MeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (sMode == "MeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return ezStatus(EZ_SUCCESS);
}
