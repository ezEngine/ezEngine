#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautPlugin/KrautTreeResource.h>
#include <Foundation/Math/Random.h>

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

  ezKrautTreeAssetProperties* pProp = GetProperties();

  ezRandom rnd;
  rnd.InitializeFromCurrentTime();
  
  ezKrautTreeResourceDescriptor desc;

  ezGeometry geo;
  geo.AddCylinder(rnd.DoubleMinMax(0.1f, 0.3f), rnd.DoubleMinMax(0.3f, 0.4f), rnd.DoubleMinMax(1.0f, 2.5f), true, false, rnd.IntMinMax(8, 32),
                  ezColor::SaddleBrown);

  geo.TriangulatePolygons();

  desc.m_Positions.Reserve(geo.GetVertices().GetCount());
  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Positions.PushBack(vtx.m_vPosition);
  }

  desc.m_TriangleIndices.Reserve(geo.GetPolygons().GetCount() * 3);
  for (const auto& poly : geo.GetPolygons())
  {
    desc.m_TriangleIndices.PushBack(poly.m_Vertices[0]);
    desc.m_TriangleIndices.PushBack(poly.m_Vertices[1]);
    desc.m_TriangleIndices.PushBack(poly.m_Vertices[2]);
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
