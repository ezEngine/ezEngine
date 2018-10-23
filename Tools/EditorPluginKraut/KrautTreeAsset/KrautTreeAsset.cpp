#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocument::ezKrautTreeAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezKrautTreeAssetProperties>(szDocumentPath, true)
{
}

const char* ezKrautTreeAssetDocument::QueryAssetType() const
{
  return "Kraut Tree";
}

//////////////////////////////////////////////////////////////////////////

enum ezKrautImportMaterialType : ezUInt8
{
  Branch = 0,
  Frond = 1,
  Leaf = 2,
};

enum ezKrautImportLodType : ezUInt8
{
  Mesh = 0,
  Impostor4Quads = 1,
  Impostor2Quads = 2,
  ImpostorBillboard = 3,
};

ezStatus ezKrautTreeAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
                                                          const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
                                                          bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  const ezKrautTreeAssetProperties* pProp = GetProperties();

  ezKrautTreeResourceDescriptor desc;

  ezFileReader krautFile;
  if (krautFile.Open(pProp->m_sKrautFile).Failed())
    return ezStatus(ezFmt("Could not open Kraut file '{0}'", pProp->m_sKrautFile));

  const ezStringBuilder sImportSourceDirectory = ezPathUtils::GetFileDirectory(pProp->m_sKrautFile);

  char signature[8];
  krautFile.ReadBytes(signature, 7);
  signature[7] = '\0';

  if (!ezStringUtils::IsEqual(signature, "{KRAUT}"))
    return ezStatus("File is not a valid Kraut file");

  ezUInt8 uiVersion = 0;
  krautFile >> uiVersion;

  if (uiVersion != 1 && uiVersion != 2)
    return ezStatus(ezFmt("Unknown Kraut file format version {0}", uiVersion));

  ezBoundingBox bbox;
  krautFile >> bbox.m_vMin;
  krautFile >> bbox.m_vMax;

  ezMath::Swap(bbox.m_vMin.y, bbox.m_vMin.z);
  ezMath::Swap(bbox.m_vMax.y, bbox.m_vMax.z);

  desc.m_Details.m_Bounds = bbox;
  desc.m_Details.m_Bounds.m_fSphereRadius *= pProp->m_fUniformScaling;
  desc.m_Details.m_Bounds.m_vBoxHalfExtends *= pProp->m_fUniformScaling;

  ezUInt8 uiNumLODs = 0;
  krautFile >> uiNumLODs;

  // m_Lods is a static array that cannot be resized
  if (uiNumLODs > desc.m_Lods.GetCapacity())
    ezLog::Error("Tree mesh contains more LODs than is supported.");

  uiNumLODs = ezMath::Min<ezUInt8>(uiNumLODs, desc.m_Lods.GetCapacity());

  const float fLodDistanceScale = pProp->m_fUniformScaling * pProp->m_fLodDistanceScale;

  ezUInt8 uiNumMaterialTypes = 0;
  krautFile >> uiNumMaterialTypes;

  EZ_ASSERT_DEV(uiNumMaterialTypes <= 3, "Unexpected number of Kraut material types: {0}", uiNumMaterialTypes);

  // remaps from combination (materialType, indexInType) to single material index
  ezHybridArray<ezHybridArray<ezUInt8, 8>, 4> MaterialToIndex;
  MaterialToIndex.SetCount(uiNumMaterialTypes);

  ezBoundingBox leafBBox;
  leafBBox.SetInvalid();

  for (ezUInt8 importMaterialType = 0; importMaterialType < uiNumMaterialTypes; ++importMaterialType)
  {
    const bool bClampTextureLookup = importMaterialType != ezKrautImportMaterialType::Branch;

    ezUInt8 uiNumMatsOfType = 0;
    krautFile >> uiNumMatsOfType;

    // ezKrautImportMaterialType
    for (ezUInt8 matOfType = 0; matOfType < uiNumMatsOfType; ++matOfType)
    {
      MaterialToIndex[importMaterialType].PushBack(desc.m_Materials.GetCount());

      auto& mat = desc.m_Materials.ExpandAndGetRef();
      mat.m_MaterialType = static_cast<ezKrautMaterialType>(importMaterialType);

      ezString sDiffuseTexture;
      krautFile >> sDiffuseTexture;

      ezString sNormalMapTexture;
      krautFile >> sNormalMapTexture;

      krautFile >> mat.m_VariationColor;

      mat.m_sDiffuseTexture =
          ImportTexture(sImportSourceDirectory, sDiffuseTexture, ezModelImporter::SemanticHint::DIFFUSE_ALPHA, bClampTextureLookup);
      mat.m_sNormalMapTexture =
          ImportTexture(sImportSourceDirectory, sNormalMapTexture, ezModelImporter::SemanticHint::NORMAL, bClampTextureLookup);
    }
  }

  ezUInt8 uiNumMeshTypes = 0;
  krautFile >> uiNumMeshTypes;

  float fPrevLodDistance = 0.0f;

  ezUInt32 uiStaticImpostorMaterialIndex = 0xFFFFFFFF;
  ezUInt32 uiBillboardImpostorMaterialIndex = 0xFFFFFFFF;

  for (ezUInt8 lodLevel = 0; lodLevel < uiNumLODs; ++lodLevel)
  {
    auto& lodData = desc.m_Lods.ExpandAndGetRef();

    lodData.m_fMinLodDistance = fPrevLodDistance;
    krautFile >> lodData.m_fMaxLodDistance;

    lodData.m_fMaxLodDistance *= fLodDistanceScale;

    if (lodData.m_fMaxLodDistance <= fPrevLodDistance)
    {
      lodData.m_fMaxLodDistance = fPrevLodDistance * 3.0f;
    }

    fPrevLodDistance = lodData.m_fMaxLodDistance;

    ezUInt8 importLodType = 0; // ezKrautImportLodType
    krautFile >> importLodType;

    switch (importLodType)
    {
      case ezKrautImportLodType::Mesh:
        lodData.m_LodType = ezKrautLodType::Mesh;
        break;
      case ezKrautImportLodType::Impostor2Quads:
        // [fallthrough]
      case ezKrautImportLodType::Impostor4Quads:
        lodData.m_LodType = ezKrautLodType::StaticImpostor;
        break;
      case ezKrautImportLodType::ImpostorBillboard:
        lodData.m_LodType = ezKrautLodType::BillboardImpostor;
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }

    ezUInt8 uiNumMatTypesUsed = 0;
    krautFile >> uiNumMatTypesUsed;

    for (ezUInt8 materialType = 0; materialType < uiNumMatTypesUsed; ++materialType)
    {
      // ezKrautImportMaterialType
      ezUInt8 uiCurMatType = 0;
      krautFile >> uiCurMatType;

      ezUInt8 uiNumMeshes = 0;
      krautFile >> uiNumMeshes;

      for (ezUInt8 uiMeshIdx = 0; uiMeshIdx < uiNumMeshes; ++uiMeshIdx)
      {
        // since we merge all vertices into one big array, the vertex indices for the triangles
        // need to be re-based relative to the previous data
        const ezUInt32 uiIndexOffset = lodData.m_Vertices.GetCount();

        ezUInt8 uiMaterialID = 0;
        krautFile >> uiMaterialID;

        ezUInt32 uiNumVertices = 0;
        krautFile >> uiNumVertices;

        ezUInt32 uiNumTriangles = 0;
        krautFile >> uiNumTriangles;

        auto& subMesh = lodData.m_SubMeshes.ExpandAndGetRef();
        subMesh.m_uiFirstTriangle = lodData.m_Triangles.GetCount();
        subMesh.m_uiNumTriangles = uiNumTriangles;

        bool bIsLeafVertex = false;

        switch (importLodType)
        {
          case ezKrautImportLodType::Mesh:
          {
            subMesh.m_uiMaterialIndex = MaterialToIndex[uiCurMatType][uiMaterialID];
            bIsLeafVertex = (desc.m_Materials[subMesh.m_uiMaterialIndex].m_MaterialType == ezKrautMaterialType::Leaf);
            break;
          }

          case ezKrautImportLodType::Impostor4Quads:
            // [fallthrough]
          case ezKrautImportLodType::Impostor2Quads:
          {
            if (uiStaticImpostorMaterialIndex == 0xFFFFFFFF)
            {
              uiStaticImpostorMaterialIndex = desc.m_Materials.GetCount();
              desc.m_Materials.ExpandAndGetRef();
              desc.m_Materials[uiStaticImpostorMaterialIndex].m_MaterialType = ezKrautMaterialType::StaticImpostor;
            }

            subMesh.m_uiMaterialIndex = uiStaticImpostorMaterialIndex;
            break;
          }

          case ezKrautImportLodType::ImpostorBillboard:
          {
            if (uiBillboardImpostorMaterialIndex == 0xFFFFFFFF)
            {
              uiBillboardImpostorMaterialIndex = desc.m_Materials.GetCount();
              desc.m_Materials.ExpandAndGetRef();
              desc.m_Materials[uiBillboardImpostorMaterialIndex].m_MaterialType = ezKrautMaterialType::BillboardImpostor;
            }

            subMesh.m_uiMaterialIndex = uiBillboardImpostorMaterialIndex;
            break;
          }

          default:
            EZ_ASSERT_NOT_IMPLEMENTED;
        }

        for (ezUInt32 v = 0; v < uiNumVertices; ++v)
        {
          auto& vtx = lodData.m_Vertices.ExpandAndGetRef();

          krautFile >> vtx.m_vPosition;
          ezMath::Swap(vtx.m_vPosition.y, vtx.m_vPosition.z);
          vtx.m_vPosition *= pProp->m_fUniformScaling;

          krautFile >> vtx.m_vTexCoord;

          krautFile >> vtx.m_vNormal;
          ezMath::Swap(vtx.m_vNormal.y, vtx.m_vNormal.z);

          krautFile >> vtx.m_vTangent;
          ezMath::Swap(vtx.m_vTangent.y, vtx.m_vTangent.z);

          if (bIsLeafVertex)
          {
            leafBBox.ExpandToInclude(vtx.m_vPosition);
            vtx.m_vTexCoord.z *= pProp->m_fUniformScaling;
          }

          if (lodData.m_LodType == ezKrautLodType::BillboardImpostor)
          {
            vtx.m_vTexCoord.z *= pProp->m_fUniformScaling;
          }

          if (uiVersion == 1)
          {
            krautFile >> vtx.m_VariationColor;
          }

          if (uiVersion >= 2)
          {
            ezUInt8 uiColorVariation;
            krautFile >> uiColorVariation;
            vtx.m_VariationColor = ezColorGammaUB(255, 255, 255, uiColorVariation);

            krautFile >> vtx.m_fAmbientOcclusion;

            for (ezUInt32 i = 0; i < 6; ++i)
            {
              // TODO: not mentioned in file format docs
              float fOtherAO;
              krautFile >> fOtherAO;
            }
          }
        }

        for (ezUInt32 t = 0; t < uiNumTriangles; ++t)
        {
          ezUInt32 idx[3];
          krautFile.ReadBytes(idx, sizeof(ezUInt32) * 3);

          auto& tri = lodData.m_Triangles.ExpandAndGetRef();

          tri.m_uiVertexIndex[0] = uiIndexOffset + idx[0];
          tri.m_uiVertexIndex[1] = uiIndexOffset + idx[2];
          tri.m_uiVertexIndex[2] = uiIndexOffset + idx[1];
        }
      }
    }
  }

  if (uiStaticImpostorMaterialIndex != 0xFFFFFFFF)
  {
    auto& mat = desc.m_Materials[uiStaticImpostorMaterialIndex];

    ezStringBuilder sDocPath = GetDocumentPath();
    ezStringBuilder sFolder = sDocPath.GetFileDirectory();
    ezStringBuilder sFile;

    sFile = sDocPath.GetFileName();
    sFile.Append("_D.tga");
    mat.m_sDiffuseTexture = ImportTexture(sFolder, sFile, ezModelImporter::SemanticHint::DIFFUSE_ALPHA, true);

    sFile = sDocPath.GetFileName();
    sFile.Append("_N.tga");
    mat.m_sNormalMapTexture = ImportTexture(sFolder, sFile, ezModelImporter::SemanticHint::NORMAL, true);
  }

  if (uiBillboardImpostorMaterialIndex != 0xFFFFFFFF)
  {
    auto& mat = desc.m_Materials[uiBillboardImpostorMaterialIndex];

    ezStringBuilder sDocPath = GetDocumentPath();
    ezStringBuilder sFolder = sDocPath.GetFileDirectory();
    ezStringBuilder sFile;

    sFile = sDocPath.GetFileName();
    sFile.Append("_D.tga");
    mat.m_sDiffuseTexture = ImportTexture(sFolder, sFile, ezModelImporter::SemanticHint::DIFFUSE_ALPHA, true);

    sFile = sDocPath.GetFileName();
    sFile.Append("_N.tga");
    mat.m_sNormalMapTexture = ImportTexture(sFolder, sFile, ezModelImporter::SemanticHint::NORMAL, true);
  }

  desc.m_Details.m_vLeafCenter = leafBBox.GetCenter();
  desc.m_Details.m_fNavMeshFootprint = pProp->m_fNavMeshFootprint;

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezKrautTreeAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

ezString ezKrautTreeAssetDocument::ImportTexture(const char* szImportSourceFolder, const char* szFilename,
                                                 ezModelImporter::SemanticHint::Enum hint, bool bTextureClamp)
{
  ezStringBuilder importTargetDirectory = GetDocumentPath();

  const bool bUseSubFolderForImportedMaterials = true;
  if (bUseSubFolderForImportedMaterials)
  {
    importTargetDirectory.Append("_data");
    importTargetDirectory.Append(ezPathUtils::OsSpecificPathSeparator);
  }
  else
    importTargetDirectory = importTargetDirectory.GetFileDirectory();

  return ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, importTargetDirectory, szFilename, hint, bTextureClamp);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocumentGenerator::ezKrautTreeAssetDocumentGenerator()
{
  AddSupportedFileType("kraut");
}

ezKrautTreeAssetDocumentGenerator::~ezKrautTreeAssetDocumentGenerator() = default;

void ezKrautTreeAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                       ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("ezKrautTreeAsset");

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "KrautTreeImport.Tree";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Kraut_Tree.png";
  }
}

ezStatus ezKrautTreeAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                     ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezKrautTreeAssetDocument* pAssetDoc = ezDynamicCast<ezKrautTreeAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
