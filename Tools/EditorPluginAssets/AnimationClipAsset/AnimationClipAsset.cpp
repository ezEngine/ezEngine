#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/Scene.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRootMotionExtractionMode, 1)
  EZ_ENUM_CONSTANTS(ezRootMotionExtractionMode::None, ezRootMotionExtractionMode::Custom, ezRootMotionExtractionMode::FromFeet, ezRootMotionExtractionMode::AvgFromFeet)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetProperties, 2, ezRTTIDefaultAllocator<ezAnimationClipAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sAnimationFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx")),
    EZ_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    EZ_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    EZ_ENUM_MEMBER_PROPERTY("RootMotionExtraction", ezRootMotionExtractionMode, m_RootMotionExtraction),
    EZ_MEMBER_PROPERTY("RootMotionVelocity", m_vCustomRootMotion),
    EZ_MEMBER_PROPERTY("Joint1", m_sJoint1),
    EZ_MEMBER_PROPERTY("Joint2", m_sJoint2),
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

  ezEditableSkeleton newSkeleton;
  EZ_SUCCEED_OR_RETURN(ezModelImporter::Importer::ImportSkeleton(newSkeleton, scene));

  ezSkeleton skeleton;
  newSkeleton.GenerateSkeleton(skeleton);

  range.BeginNextStep("Writing Result");

  const ezUInt32 uiAnimationToUse = 0;

  const auto& animClip = scene->m_AnimationClips[uiAnimationToUse];

  // first and last frame are inclusive and may be equal to pick only a single frame
  const ezUInt32 uiFirstKeyframe = ezMath::Min(pProp->m_uiFirstFrame, animClip.m_uiNumKeyframes - 1);
  const ezUInt32 uiMaxKeyframes = animClip.m_uiNumKeyframes - uiFirstKeyframe;
  const ezUInt32 uiNumKeyframes = ezMath::Min((pProp->m_uiNumFrames == 0) ? uiMaxKeyframes : pProp->m_uiNumFrames, uiMaxKeyframes);

  const bool bHasRootMotion = pProp->m_RootMotionExtraction != ezRootMotionExtractionMode::None;

  ezAnimationClipResourceDescriptor anim;
  anim.Configure(animClip.m_JointAnimations.GetCount(), uiNumKeyframes, animClip.m_uiFramesPerSecond, bHasRootMotion);

  for (ezUInt32 b = 0; b < anim.GetNumJoints(); ++b)
  {
    ezHashedString hs;
    hs.Assign(animClip.m_JointAnimations[b].m_sJointName.GetData());
    ezUInt16 uiJointIdx = anim.AddJointName(hs);

    ezTransform* pJointTransforms = anim.GetJointKeyframes(uiJointIdx);

    ezMemoryUtils::Copy(pJointTransforms, &animClip.m_JointAnimations[b].m_Keyframes.GetData()[uiFirstKeyframe], uiNumKeyframes);
  }

  if (anim.HasRootMotion())
  {
    if (pProp->m_RootMotionExtraction == ezRootMotionExtractionMode::Custom)
    {
      ApplyCustomRootMotion(anim);
    }
    else if (pProp->m_RootMotionExtraction == ezRootMotionExtractionMode::FromFeet)
    {
      ExtractRootMotionFromFeet(anim, skeleton);
    }
    else if (pProp->m_RootMotionExtraction == ezRootMotionExtractionMode::AvgFromFeet)
    {
      ExtractRootMotionFromFeet(anim, skeleton);
      MakeRootMotionConstantAverage(anim);
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

void ezAnimationClipAssetDocument::ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const
{
  const ezAnimationClipAssetProperties* pProp = GetProperties();
  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
  ezTransform* pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);

  const ezVec3 vKeyframeMotion = pProp->m_vCustomRootMotion / (float)anim.GetFramesPerSecond();
  const ezTransform rootTransform(vKeyframeMotion);

  for (ezUInt32 kf = 0; kf < anim.GetNumFrames(); ++kf)
  {
    pRootTransforms[kf] = rootTransform;
  }
}

void ezAnimationClipAssetDocument::ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const
{
  const ezAnimationClipAssetProperties* pProp = GetProperties();
  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
  ezTransform* pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);

  ezUInt32 uiFoot1, uiFoot2;
  if (skeleton.FindJointByName(ezTempHashedString(pProp->m_sJoint1.GetData()), uiFoot1).Failed() ||
      skeleton.FindJointByName(ezTempHashedString(pProp->m_sJoint2.GetData()), uiFoot2).Failed())
  {
    ezLog::Error("Joints '{0}' and '{1}' could not be found in animation clip", pProp->m_sJoint1, pProp->m_sJoint2);
    return;
  }

  ezAnimationPose pose;
  pose.Configure(skeleton);

  ezVec3 lastFootPos1(0), lastFootPos2(0);

  // init last foot position with very last frame data
  {
    pose.SetToBindPoseInLocalSpace(skeleton);
    anim.SetPoseToKeyframe(pose, skeleton, anim.GetNumFrames() - 1);
    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);

    lastFootPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
    lastFootPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
  }

  ezInt32 lastFootDown = (lastFootPos1.z < lastFootPos2.z) ? 1 : 2;

  ezHybridArray<ezUInt16, 32> unknownMotion;

  for (ezUInt16 frame = 0; frame < anim.GetNumFrames(); ++frame)
  {
    pose.SetToBindPoseInLocalSpace(skeleton);
    anim.SetPoseToKeyframe(pose, skeleton, frame);
    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);

    const ezVec3 footPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
    const ezVec3 footPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();

    const ezVec3 footDir1 = footPos1 - lastFootPos1;
    const ezVec3 footDir2 = footPos2 - lastFootPos2;

    ezVec3 rootMotion(0);

    const ezInt32 curFootDown = (footPos1.z < footPos2.z) ? 1 : 2;

    if (lastFootDown == curFootDown)
    {
      if (curFootDown == 1)
        rootMotion = -footDir1;
      else
        rootMotion = -footDir2;

      rootMotion.z = 0;
      pRootTransforms[frame] = ezTransform(rootMotion);
    }
    else
    {
      // set them via average later on
      unknownMotion.PushBack(frame);
      pRootTransforms[frame].SetIdentity();
    }

    lastFootDown = curFootDown;
    lastFootPos1 = footPos1;
    lastFootPos2 = footPos2;
  }

  // fix unknown motion frames
  for (ezUInt16 crossedFeet : unknownMotion)
  {
    const ezUInt16 prevFrame = (crossedFeet > 0) ? (crossedFeet - 1) : anim.GetNumFrames() - 1;
    const ezUInt16 nextFrame = (crossedFeet + 1) % anim.GetNumFrames();

    const ezVec3 avgTranslation = ezMath::Lerp(pRootTransforms[prevFrame].m_vPosition, pRootTransforms[nextFrame].m_vPosition, 0.5f);

    pRootTransforms[crossedFeet] = ezTransform(avgTranslation);
  }

  const ezUInt16 numFrames = anim.GetNumFrames();

  ezHybridArray<ezVec3, 32> translations;
  translations.SetCount(numFrames);

  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
  {
    translations[thisFrame] = pRootTransforms[thisFrame].m_vPosition;
  }

  // do some smoothing
  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
  {
    const ezUInt16 prevFrame2 = (numFrames + thisFrame - 2) % numFrames;
    const ezUInt16 prevFrame = (numFrames + thisFrame - 1) % numFrames;
    const ezUInt16 nextFrame = (thisFrame + 1) % numFrames;
    const ezUInt16 nextFrame2 = (thisFrame + 2) % numFrames;

    const ezVec3 smoothedTranslation = (translations[prevFrame2] + translations[prevFrame] + translations[thisFrame] +
                                        translations[nextFrame] + translations[nextFrame2]) *
                                       0.2f;

    pRootTransforms[thisFrame].m_vPosition = smoothedTranslation;
  }

  // for (ezUInt32 i = 0; i < anim.GetNumFrames(); ++i)
  //{
  //  ezLog::Info("Motion {0}: {1} | {2}", ezArgI(i, 3), ezArgF(pRootTransforms[i].m_vPosition.x, 1),
  //              ezArgF(pRootTransforms[i].m_vPosition.y, 1));
  //}
}

void ezAnimationClipAssetDocument::MakeRootMotionConstantAverage(ezAnimationClipResourceDescriptor& anim) const
{
  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
  ezTransform* pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
  const ezUInt16 numFrames = anim.GetNumFrames();

  ezVec3 avgFootTranslation(0);

  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
  {
    avgFootTranslation += pRootTransforms[thisFrame].m_vPosition;
  }

  avgFootTranslation /= numFrames;

  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
  {
    pRootTransforms[thisFrame].m_vPosition = avgFootTranslation;
  }
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
