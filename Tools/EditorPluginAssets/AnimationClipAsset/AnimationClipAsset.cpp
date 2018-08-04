#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetProperties, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sAnimationFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx")),
    EZ_MEMBER_PROPERTY("Skeleton", m_sSkeletonResourceFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("LastFrame", m_uiLastFrame)->AddAttributes(new ezDefaultValueAttribute(0xFFFFFFFF)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationClipAssetProperties::ezAnimationClipAssetProperties()
{
  m_uiFirstFrame = 0;
  m_uiLastFrame = 0xFFFFFFFF;
}

ezAnimationClipAssetDocument::ezAnimationClipAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezAnimationClipAssetProperties>(szDocumentPath, true)
{
}

ezStatus ezAnimationClipAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                              const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimationClipAssetProperties* pProp = GetProperties();

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Mesh");

  range.BeginNextStep("Writing Result");

  ezAnimationClipResourceDescriptor anim;
  anim.Configure(100, 45, 30);

  for (ezUInt32 f = 0; f < anim.GetNumFrames(); ++f)
  {
    const ezAngle rot = (ezAngle::Degree(360) / anim.GetNumFrames()) * f;
    ezMat4 mRot;
    mRot.SetRotationMatrixZ(rot);

    ezMat4* pBones = anim.GetFirstBones(f);

    for (ezUInt32 b = 0; b < anim.GetNumBones(); ++b)
    {
      pBones[b] = mRot;
    }
  }

  anim.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimationClipAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationClipAssetDocumentGenerator::ezAnimationClipAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
}

ezAnimationClipAssetDocumentGenerator::~ezAnimationClipAssetDocumentGenerator() {}

void ezAnimationClipAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                           ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animation_Clip.png";
  }
}

ezStatus ezAnimationClipAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                         ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateOrOpenDocument(true, info.m_sOutputFileAbsolute, false, false);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezAnimationClipAssetDocument* pAssetDoc = ezDynamicCast<ezAnimationClipAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezAnimationClipAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
