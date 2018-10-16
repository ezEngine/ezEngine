#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
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

    ezUInt8 uiNumLODs = 0;
    krautFile >> uiNumLODs;

    ezUInt8 uiNumMaterialTypes = 0;
    krautFile >> uiNumMaterialTypes;

    for (ezUInt8 type = 0; type < uiNumMaterialTypes; ++type)
    {
      ezUInt8 uiNumMatsOfType = 0;
      krautFile >> uiNumMatsOfType;

      for (ezUInt8 matOfType = 0; matOfType < uiNumMatsOfType; ++matOfType)
      {
        ezString sDiffuseTexture;
        krautFile >> sDiffuseTexture;

        ezString sNormalMapTexture;
        krautFile >> sNormalMapTexture;

        ezColorGammaUB variationColor;
        krautFile >> variationColor;
      }
    }

    ezUInt8 uiNumMeshTypes = 0;
    krautFile >> uiNumMeshTypes;

    for (ezUInt8 lodLevel = 0; lodLevel < uiNumLODs; ++lodLevel)
    {
      float fLodDistance = 0;
      krautFile >> fLodDistance;

      ezUInt8 lodType = 0;
      krautFile >> lodType; // 0 == full mesh, 1 == 4 quad impostor, 2 == 2 quad impostor, 3 == billboard impostor

      ezUInt8 uiNumMatTypesUsed = 0;
      krautFile >> uiNumMatTypesUsed;

      for (ezUInt8 materialType = 0; materialType < uiNumMatTypesUsed; ++materialType)
      {
        ezUInt8 uiCurMatType = 0;
        krautFile >> uiCurMatType;

        ezUInt8 uiNumMeshes = 0;
        krautFile >> uiNumMeshes;

        const bool bUseMeshData = lodLevel == 0 && uiCurMatType == 0;

        for (ezUInt8 uiMeshIdx = 0; uiMeshIdx < uiNumMeshes; ++uiMeshIdx)
        {
          const ezUInt32 uiIndexOffset = desc.m_TriangleIndices.GetCount();

          ezUInt8 uiMaterialID = 0;
          krautFile >> uiMaterialID;

          ezUInt32 uiNumVertices = 0;
          krautFile >> uiNumVertices;

          ezUInt32 uiNumTriangles = 0;
          krautFile >> uiNumTriangles;

          for (ezUInt32 v = 0; v < uiNumVertices; ++v)
          {
            ezVec3 pos, texcoord, normal, tangent;
            krautFile >> pos;
            krautFile >> texcoord;
            krautFile >> normal;
            krautFile >> tangent;

            ezColorGammaUB variationColor;
            krautFile >> variationColor;

            ezMath::Swap(pos.y, pos.z);
            pos *= pProp->m_fUniformScaling;

            if (bUseMeshData)
            {
              desc.m_Positions.PushBack(pos);
            }
          }

          for (ezUInt32 t = 0; t < uiNumTriangles; ++t)
          {
            ezUInt32 idx[3];
            krautFile.ReadBytes(idx, sizeof(ezUInt32) * 3);

            if (bUseMeshData)
            {
              desc.m_TriangleIndices.PushBack(uiIndexOffset + idx[0]);
              desc.m_TriangleIndices.PushBack(uiIndexOffset + idx[2]);
              desc.m_TriangleIndices.PushBack(uiIndexOffset + idx[1]);
            }
          }
        }
      }
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

//////////////////////////////////////////////////////////////////////////

// EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetDocumentGenerator>)
// EZ_END_DYNAMIC_REFLECTED_TYPE;
//
// ezKrautTreeAssetDocumentGenerator::ezKrautTreeAssetDocumentGenerator()
//{
//  AddSupportedFileType("kraut");
//}
//
// ezKrautTreeAssetDocumentGenerator::~ezKrautTreeAssetDocumentGenerator() = default;
//
// void ezKrautTreeAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
//                                                           ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
//{
//  ezStringBuilder baseOutputFile = szParentDirRelativePath;
//  baseOutputFile.ChangeFileExtension("ezKrautTreeAsset");
//
//  {
//    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
//    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
//    info.m_sName = "KrautTreeImport.TriangleMesh";
//    info.m_sOutputFileParentRelative = baseOutputFile;
//    info.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
//  }
//
//  {
//    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
//    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
//    info.m_sName = "KrautTreeImport.ConvexMesh";
//    info.m_sOutputFileParentRelative = baseOutputFile;
//    info.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
//  }
//}
//
// ezStatus ezKrautTreeAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
//                                                         ezDocument*& out_pGeneratedDocument)
//{
//  auto pApp = ezQtEditorApp::GetSingleton();
//
//  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
//  if (out_pGeneratedDocument == nullptr)
//    return ezStatus("Could not create target document");
//
//  ezKrautTreeAssetDocument* pAssetDoc = ezDynamicCast<ezKrautTreeAssetDocument*>(out_pGeneratedDocument);
//  if (pAssetDoc == nullptr)
//    return ezStatus("Target document is not a valid ezKrautTreeAssetDocument");
//
//  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
//  accessor.SetValue("MeshFile", szDataDirRelativePath);
//
//  if (info.m_sName == "KrautTreeImport.ConvexMesh")
//  {
//    accessor.SetValue("MeshType", (int)ezKrautTreeType::ConvexHull);
//  }
//  else
//  {
//    accessor.SetValue("MeshType", (int)ezKrautTreeType::TriangleMesh);
//  }
//
//  return ezStatus(EZ_SUCCESS);
//}
