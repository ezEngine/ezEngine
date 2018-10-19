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

  {
    char signature[8];
    krautFile.ReadBytes(signature, 7);
    signature[7] = '\0';

    if (!ezStringUtils::IsEqual(signature, "{KRAUT}"))
      return ezStatus("File is not a valid Kraut file");

    ezUInt8 uiVersion = 0;
    krautFile >> uiVersion;

    if (uiVersion != 1)
      return ezStatus(ezFmt("Unknown Kraut file format version {0}", uiVersion));

    ezBoundingBox bbox;
    krautFile >> bbox.m_vMin;
    krautFile >> bbox.m_vMax;

    ezMath::Swap(bbox.m_vMin.y, bbox.m_vMin.z);
    ezMath::Swap(bbox.m_vMax.y, bbox.m_vMax.z);

    desc.m_Bounds = bbox;

    ezUInt8 uiNumLODs = 0;
    krautFile >> uiNumLODs;

    // m_Lods is a static array that cannot be resized
    if (uiNumLODs > desc.m_Lods.GetCapacity())
      ezLog::Error("Tree mesh contains more LODs than is supported.");

    uiNumLODs = ezMath::Min<ezUInt8>(uiNumLODs, desc.m_Lods.GetCapacity());

    ezUInt8 uiNumMaterialTypes = 0;
    krautFile >> uiNumMaterialTypes;

    // remaps from combination (materialType, indexInType) to single material index
    ezHybridArray<ezHybridArray<ezUInt8, 8>, 4> MaterialToIndex;
    MaterialToIndex.SetCount(uiNumMaterialTypes);

    for (ezUInt8 type = 0; type < uiNumMaterialTypes; ++type)
    {
      ezUInt8 uiNumMatsOfType = 0;
      krautFile >> uiNumMatsOfType;

      for (ezUInt8 matOfType = 0; matOfType < uiNumMatsOfType; ++matOfType)
      {
        MaterialToIndex[type].PushBack(desc.m_Materials.GetCount());

        auto& mat = desc.m_Materials.ExpandAndGetRef();
        mat.m_uiMaterialType = type;

        ezString sDiffuseTexture;
        krautFile >> sDiffuseTexture;

        ezString sNormalMapTexture;
        krautFile >> sNormalMapTexture;

        krautFile >> mat.m_VariationColor;

        mat.m_sDiffuseTexture = ImportTexture(sImportSourceDirectory, sDiffuseTexture);
        mat.m_sNormalMapTexture = ImportTexture(sImportSourceDirectory, sNormalMapTexture);
      }
    }

    ezUInt8 uiNumMeshTypes = 0;
    krautFile >> uiNumMeshTypes;

    float fPrevLodDistance = 0.0f;

    bool bUseImpostorTexture = false;
    const ezUInt32 uiImpostorMaterialIndex = desc.m_Materials.GetCount();

    for (ezUInt8 lodLevel = 0; lodLevel < uiNumLODs; ++lodLevel)
    {
      auto& lodData = desc.m_Lods.ExpandAndGetRef();

      lodData.m_fMinLodDistance = fPrevLodDistance;
      krautFile >> lodData.m_fMaxLodDistance;
      fPrevLodDistance = lodData.m_fMaxLodDistance;

      ezUInt8 lodType = 0;
      krautFile >> lodType; // 0 == full mesh, 1 == 4 quad impostor, 2 == 2 quad impostor, 3 == billboard impostor

      ezUInt8 uiNumMatTypesUsed = 0;
      krautFile >> uiNumMatTypesUsed;

      for (ezUInt8 materialType = 0; materialType < uiNumMatTypesUsed; ++materialType)
      {
        ezUInt8 uiCurMatType = 0;
        krautFile >> uiCurMatType; // 0 == branch, 1 == frond, 2 == leaf

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

          if (lodType == 0)
          {
            subMesh.m_uiMaterialIndex = MaterialToIndex[uiCurMatType][uiMaterialID];
          }
          else
          {
            bUseImpostorTexture = true;
            subMesh.m_uiMaterialIndex = uiImpostorMaterialIndex;
          }

          for (ezUInt32 v = 0; v < uiNumVertices; ++v)
          {
            auto& vtx = lodData.m_Vertices.ExpandAndGetRef();

            krautFile >> vtx.m_vPosition;
            krautFile >> vtx.m_vTexCoord;
            krautFile >> vtx.m_vNormal;
            krautFile >> vtx.m_vTangent;
            krautFile >> vtx.m_VariationColor;

            ezMath::Swap(vtx.m_vPosition.y, vtx.m_vPosition.z);
            vtx.m_vPosition *= pProp->m_fUniformScaling;

            ezMath::Swap(vtx.m_vNormal.y, vtx.m_vNormal.z);
            ezMath::Swap(vtx.m_vTangent.y, vtx.m_vTangent.z);
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

    if (bUseImpostorTexture)
    {
      auto& mat = desc.m_Materials.ExpandAndGetRef();
      mat.m_uiMaterialType = 1; // 'frond'
      mat.m_VariationColor = ezColor::White;

      ezStringBuilder sDocPath = GetDocumentPath();
      ezStringBuilder sFolder = sDocPath.GetFileDirectory();
      ezStringBuilder sFile;

      sFile = sDocPath.GetFileName();
      sFile.Append("_D.tga");
      mat.m_sDiffuseTexture = ImportTexture(sFolder, sFile);

      sFile = sDocPath.GetFileName();
      sFile.Append("_N.tga");
      mat.m_sNormalMapTexture = ImportTexture(sFolder, sFile);
    }
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezKrautTreeAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

ezString ezKrautTreeAssetDocument::ImportTexture(const char* szImportSourceFolder, const char* szFilename)
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

  return ezMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, importTargetDirectory, szFilename,
                                                   ezModelImporter::SemanticHint::DIFFUSE_ALPHA);
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
