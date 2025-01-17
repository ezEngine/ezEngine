#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRootMotionSource, 1)
  EZ_ENUM_CONSTANTS(ezRootMotionSource::None, ezRootMotionSource::Constant)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetProperties, 3, ezRTTIDefaultAllocator<ezAnimationClipAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new ezFileBrowserAttribute("Select Animation", ezFileBrowserAttribute::MeshesWithAnimations)),
    EZ_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", ezDependencyFlags::None)),
    EZ_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract),
    EZ_ARRAY_MEMBER_PROPERTY("AvailableClips", m_AvailableClips)->AddAttributes(new ezReadOnlyAttribute, new ezContainerAttribute(false, false, false)),
    EZ_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    EZ_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    EZ_MEMBER_PROPERTY("Additive", m_bAdditive),
    EZ_ENUM_MEMBER_PROPERTY("RootMotion", ezRootMotionSource, m_RootMotionMode),
    EZ_MEMBER_PROPERTY("ConstantRootMotion", m_vConstantRootMotion),
    EZ_MEMBER_PROPERTY("EventTrack", m_EventTrack)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocument, 5, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimationClipAssetProperties::ezAnimationClipAssetProperties() = default;
ezAnimationClipAssetProperties::~ezAnimationClipAssetProperties() = default;

void ezAnimationClipAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != ezGetStaticRTTI<ezAnimationClipAssetProperties>())
    return;

  auto& props = *e.m_pPropertyStates;

  const ezInt64 motionType = e.m_pObject->GetTypeAccessor().GetValue("RootMotion").ConvertTo<ezInt64>();

  switch (motionType)
  {
    case ezRootMotionSource::Constant:
      props["ConstantRootMotion"].m_Visibility = ezPropertyUiState::Default;
      break;

    default:
      props["ConstantRootMotion"].m_Visibility = ezPropertyUiState::Invisible;
      break;
  }
}

ezAnimationClipAssetDocument::ezAnimationClipAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezAnimationClipAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

void ezAnimationClipAssetDocument::SetCommonAssetUiState(ezCommonAssetUiState::Enum state, double value)
{
  switch (state)
  {
    case ezCommonAssetUiState::SimulationSpeed:
      m_fSimulationSpeed = value;
      break;
    default:
      break;
  }

  // handles standard booleans and broadcasts the event
  return SUPER::SetCommonAssetUiState(state, value);
}

double ezAnimationClipAssetDocument::GetCommonAssetUiState(ezCommonAssetUiState::Enum state) const
{
  switch (state)
  {
    case ezCommonAssetUiState::SimulationSpeed:
      return m_fSimulationSpeed;
    default:
      break;
  }

  return SUPER::GetCommonAssetUiState(state);
}

ezTransformStatus ezAnimationClipAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimationClipAssetProperties* pProp = GetProperties();

  ezAnimationClipResourceDescriptor desc;

  range.BeginNextStep("Importing Animations");

  ezStringBuilder sAbsFilename = pProp->m_sSourceFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return ezStatus("No known importer for this file type.");

  ezEditableSkeleton skeleton;

  ezModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  // opt.m_pSkeletonOutput = &skeleton; // TODO: may be needed later to optimize the clip
  opt.m_pAnimationOutput = &desc;
  opt.m_bAdditiveAnimation = pProp->m_bAdditive;
  opt.m_sAnimationToImport = pProp->m_sAnimationClipToExtract;
  opt.m_uiFirstAnimKeyframe = pProp->m_uiFirstFrame;
  opt.m_uiNumAnimKeyframes = pProp->m_uiNumFrames;

  const ezResult res = pImporter->Import(opt);

  if (res.Succeeded())
  {
    if (pProp->m_RootMotionMode == ezRootMotionSource::Constant)
    {
      desc.m_vConstantRootMotion = pProp->m_vConstantRootMotion;
    }

    range.BeginNextStep("Writing Result");

    pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

    EZ_SUCCEED_OR_RETURN(desc.Serialize(stream));
  }

  // if we found information about animation clips, update the UI, even if the transform failed
  if (!pImporter->m_OutputAnimationNames.IsEmpty())
  {
    pProp->m_AvailableClips.SetCount(pImporter->m_OutputAnimationNames.GetCount());
    for (ezUInt32 clip = 0; clip < pImporter->m_OutputAnimationNames.GetCount(); ++clip)
    {
      pProp->m_AvailableClips[clip] = pImporter->m_OutputAnimationNames[clip];
    }

    // merge the new data with the actual asset document
    ApplyNativePropertyChangesToObjectManager(true);
  }

  if (res.Failed())
    return ezStatus("Model importer was unable to read this asset.");

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezAnimationClipAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails would remain black for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }

  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

ezUuid ezAnimationClipAssetDocument::InsertEventTrackCpAt(ezInt64 iTickX, const char* szValue)
{
  ezObjectCommandAccessor accessor(GetCommandHistory());
  ezObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const ezAbstractProperty* pTrackProp = ezGetStaticRTTI<ezAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  ezUuid trackGuid = accessor.Get<ezUuid>(GetPropertyObject(), pTrackProp);

  ezUuid newObjectGuid;
  EZ_VERIFY(acc.AddObjectByName(accessor.GetObject(trackGuid), "ControlPoints", -1, ezGetStaticRTTI<ezEventTrackControlPointData>(), newObjectGuid).Succeeded(), "");
  const ezDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  EZ_VERIFY(acc.SetValueByName(pCPObj, "Tick", iTickX).Succeeded(), "");
  EZ_VERIFY(acc.SetValueByName(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

// void ezAnimationClipAssetDocument::ApplyCustomRootMotion(ezAnimationClipResourceDescriptor& anim) const
//{
//  const ezAnimationClipAssetProperties* pProp = GetProperties();
//  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  ezArrayPtr<ezTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const ezVec3 vKeyframeMotion = pProp->m_vCustomRootMotion / (float)anim.GetFramesPerSecond();
//  const ezTransform rootTransform(vKeyframeMotion);
//
//  for (ezUInt32 kf = 0; kf < anim.GetNumFrames(); ++kf)
//  {
//    pRootTransforms[kf] = rootTransform;
//  }
//}
//
// void ezAnimationClipAssetDocument::ExtractRootMotionFromFeet(ezAnimationClipResourceDescriptor& anim, const ezSkeleton& skeleton) const
//{
//  const ezAnimationClipAssetProperties* pProp = GetProperties();
//  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  ezArrayPtr<ezTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const ezUInt16 uiFoot1 = skeleton.FindJointByName(ezTempHashedString(pProp->m_sJoint1.GetData()));
//  const ezUInt16 uiFoot2 = skeleton.FindJointByName(ezTempHashedString(pProp->m_sJoint2.GetData()));
//
//  if (uiFoot1 == ezInvalidJointIndex || uiFoot2 == ezInvalidJointIndex)
//  {
//    ezLog::Error("Joints '{0}' and '{1}' could not be found in animation clip", pProp->m_sJoint1, pProp->m_sJoint2);
//    return;
//  }
//
//  ezAnimationPose pose;
//  pose.Configure(skeleton);
//
//  ezVec3 lastFootPos1(0), lastFootPos2(0);
//
//  // init last foot position with very last frame data
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, anim.GetNumFrames() - 1);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    lastFootPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    lastFootPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//  }
//
//  ezInt32 lastFootDown = (lastFootPos1.z < lastFootPos2.z) ? 1 : 2;
//
//  ezHybridArray<ezUInt16, 32> unknownMotion;
//
//  for (ezUInt16 frame = 0; frame < anim.GetNumFrames(); ++frame)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, frame);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    const ezVec3 footPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    const ezVec3 footPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//
//    const ezVec3 footDir1 = footPos1 - lastFootPos1;
//    const ezVec3 footDir2 = footPos2 - lastFootPos2;
//
//    ezVec3 rootMotion(0);
//
//    const ezInt32 curFootDown = (footPos1.z < footPos2.z) ? 1 : 2;
//
//    if (lastFootDown == curFootDown)
//    {
//      if (curFootDown == 1)
//        rootMotion = -footDir1;
//      else
//        rootMotion = -footDir2;
//
//      rootMotion.z = 0;
//      pRootTransforms[frame] = ezTransform(rootMotion);
//    }
//    else
//    {
//      // set them via average later on
//      unknownMotion.PushBack(frame);
//      pRootTransforms[frame].SetIdentity();
//    }
//
//    lastFootDown = curFootDown;
//    lastFootPos1 = footPos1;
//    lastFootPos2 = footPos2;
//  }
//
//  // fix unknown motion frames
//  for (ezUInt16 crossedFeet : unknownMotion)
//  {
//    const ezUInt16 prevFrame = (crossedFeet > 0) ? (crossedFeet - 1) : anim.GetNumFrames() - 1;
//    const ezUInt16 nextFrame = (crossedFeet + 1) % anim.GetNumFrames();
//
//    const ezVec3 avgTranslation = ezMath::Lerp(pRootTransforms[prevFrame].m_vPosition, pRootTransforms[nextFrame].m_vPosition, 0.5f);
//
//    pRootTransforms[crossedFeet] = ezTransform(avgTranslation);
//  }
//
//  const ezUInt16 numFrames = anim.GetNumFrames();
//
//  ezHybridArray<ezVec3, 32> translations;
//  translations.SetCount(numFrames);
//
//  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    translations[thisFrame] = pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  // do some smoothing
//  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    const ezUInt16 prevFrame2 = (numFrames + thisFrame - 2) % numFrames;
//    const ezUInt16 prevFrame = (numFrames + thisFrame - 1) % numFrames;
//    const ezUInt16 nextFrame = (thisFrame + 1) % numFrames;
//    const ezUInt16 nextFrame2 = (thisFrame + 2) % numFrames;
//
//    const ezVec3 smoothedTranslation =
//      (translations[prevFrame2] + translations[prevFrame] + translations[thisFrame] + translations[nextFrame] + translations[nextFrame2]) * 0.2f;
//
//    pRootTransforms[thisFrame].m_vPosition = smoothedTranslation;
//  }
//
//  // for (ezUInt32 i = 0; i < anim.GetNumFrames(); ++i)
//  //{
//  //  ezLog::Info("Motion {0}: {1} | {2}", ezArgI(i, 3), ezArgF(pRootTransforms[i].m_vPosition.x, 1),
//  //              ezArgF(pRootTransforms[i].m_vPosition.y, 1));
//  //}
//}
//
// void ezAnimationClipAssetDocument::MakeRootMotionConstantAverage(ezAnimationClipResourceDescriptor& anim) const
//{
//  const ezUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  ezArrayPtr<ezTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//  const ezUInt16 numFrames = anim.GetNumFrames();
//
//  ezVec3 avgFootTranslation(0);
//
//  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    avgFootTranslation += pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  avgFootTranslation /= numFrames;
//
//  for (ezUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    pRootTransforms[thisFrame].m_vPosition = avgFootTranslation;
//  }
//}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationClipAssetDocumentGenerator::ezAnimationClipAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezAnimationClipAssetDocumentGenerator::~ezAnimationClipAssetDocumentGenerator() = default;

void ezAnimationClipAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_Single";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }

  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_All";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }
}

ezStatus ezAnimationClipAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  ezOSFile::FindFreeFilename(sOutFile);

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezStringBuilder title;
  title.SetFormat("Select Preview Mesh for Animation Clip '{}'", sInputFileAbs.GetFileName());

  ezStringBuilder sPreviewMesh;

  ezQtAssetBrowserDlg dlg(nullptr, ezUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skinned", title);
  if (dlg.exec() != 0)
  {
    if (dlg.GetSelectedAssetGuid().IsValid())
    {
      ezConversionUtils::ToString(dlg.GetSelectedAssetGuid(), sPreviewMesh);
    }
  }

  if (sMode == "AnimationClipImport_Single")
  {
    ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
    if (pDoc == nullptr)
      return ezStatus("Could not create target document");

    out_generatedDocuments.PushBack(pDoc);

    ezAnimationClipAssetDocument* pAssetDoc = ezDynamicCast<ezAnimationClipAssetDocument*>(pDoc);

    auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
    accessor.SetValue("File", sInputFileRel.GetView());
    accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

    return ezStatus(EZ_SUCCESS);
  }

  if (sMode == "AnimationClipImport_All")
  {
    ezModelImporter2::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;

    ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(opt.m_sSourceFile);
    if (pImporter == nullptr)
      return ezStatus("No known importer for this file type.");

    if (pImporter->Import(opt).Failed())
      return ezStatus("Failed to import asset.");

    ezStringBuilder sFilename;
    ezStringBuilder sOutFile2;

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      sFilename = clip;
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);
      ezOSFile::FindFreeFilename(sOutFile2);

      ezDocument* pDoc = pApp->CreateDocument(sOutFile2, ezDocumentFlags::None);
      if (pDoc == nullptr)
        return ezStatus("Could not create target document");

      out_generatedDocuments.PushBack(pDoc);

      ezAnimationClipAssetDocument* pAssetDoc = ezDynamicCast<ezAnimationClipAssetDocument*>(pDoc);

      auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
      accessor.SetValue("File", sInputFileRel.GetView());
      accessor.SetValue("UseAnimationClip", clip);
      accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());
    }

    return ezStatus(EZ_SUCCESS);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezStatus(EZ_FAILURE);
}
