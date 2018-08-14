#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/Scene.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetProperties, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sAnimationFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx")),
    EZ_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    EZ_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    EZ_MEMBER_PROPERTY("RootMotionVelocity", m_vCustomRootMotion),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationClipAssetProperties::ezAnimationClipAssetProperties()
{
  m_vCustomRootMotion.SetZero();
}

ezAnimationClipAssetDocument::ezAnimationClipAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezAnimationClipAssetProperties>(szDocumentPath, true)
{
}

using namespace ezModelImporter;

static ezStatus ImportAnimation(const char* filename, ezSharedPtr<ezModelImporter::Scene>& outScene)
{
  ezStringBuilder sAbsFilename = filename;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  outScene = Importer::GetSingleton()->ImportScene(sAbsFilename, ImportFlags::Skeleton | ImportFlags::Animations);

  if (outScene == nullptr)
    return ezStatus(ezFmt("Input file '{0}' could not be imported", filename));

  if (outScene->m_AnimationClips.IsEmpty())
    return ezStatus("File does not contain any animations");

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimationClipAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                              const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimationClipAssetProperties* pProp = GetProperties();

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Animations");

  ezSharedPtr<Scene> scene;
  EZ_SUCCEED_OR_RETURN(ImportAnimation(pProp->m_sAnimationFile, scene));

  range.BeginNextStep("Writing Result");

  const ezUInt32 uiAnimationToUse = 0;

  const auto& animClip = scene->m_AnimationClips[uiAnimationToUse];

  // first and last frame are inclusive and may be equal to pick only a single frame
  const ezUInt32 uiFirstKeyframe = ezMath::Min(pProp->m_uiFirstFrame, animClip.m_uiNumKeyframes - 1);
  const ezUInt32 uiMaxKeyframes = animClip.m_uiNumKeyframes - uiFirstKeyframe;
  const ezUInt32 uiNumKeyframes = ezMath::Min((pProp->m_uiNumFrames == 0) ? uiMaxKeyframes : pProp->m_uiNumFrames, uiMaxKeyframes);

  ezAnimationClipResourceDescriptor anim;
  anim.Configure(animClip.m_JointAnimations.GetCount(), uiNumKeyframes, animClip.m_uiFramesPerSecond, !pProp->m_vCustomRootMotion.IsZero());

  if (anim.HasRootMotion())
  {
    const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
    ezTransform* pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);

    if (!pProp->m_vCustomRootMotion.IsZero())
    {
      const ezVec3 vKeyframeMotion = pProp->m_vCustomRootMotion / (float)animClip.m_uiFramesPerSecond;
      const ezTransform rootTransform(vKeyframeMotion);

      for (ezUInt32 kf = 0; kf < uiNumKeyframes; ++kf)
      {
        pRootTransforms[kf] = rootTransform;
      }
    }
  }

  for (ezUInt32 b = 0; b < anim.GetNumJoints(); ++b)
  {
    ezHashedString hs;
    hs.Assign(animClip.m_JointAnimations[b].m_sJointName.GetData());
    ezUInt16 uiJointIdx = anim.AddJointName(hs);

    ezTransform* pJointTransforms = anim.GetJointKeyframes(uiJointIdx);

    ezMemoryUtils::Copy(pJointTransforms, &animClip.m_JointAnimations[b].m_Keyframes.GetData()[uiFirstKeyframe], uiNumKeyframes);
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
