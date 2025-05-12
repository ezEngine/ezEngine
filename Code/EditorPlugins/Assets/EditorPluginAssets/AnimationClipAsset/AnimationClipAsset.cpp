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

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAdditiveAnimationReference, 1)
  EZ_ENUM_CONSTANTS(ezAdditiveAnimationReference::FirstKeyFrame, ezAdditiveAnimationReference::LastKeyFrame)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetProperties, 3, ezRTTIDefaultAllocator<ezAnimationClipAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new ezFileBrowserAttribute("Select Animation", ezFileBrowserAttribute::MeshesWithAnimations)),
    EZ_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", ezDependencyFlags::Thumbnail)),
    EZ_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract),
    EZ_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    EZ_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    EZ_MEMBER_PROPERTY("Additive", m_bAdditive),
    EZ_ENUM_MEMBER_PROPERTY("AdditiveReference", ezAdditiveAnimationReference, m_AdditiveReference),
    EZ_ENUM_MEMBER_PROPERTY("RootMotion", ezRootMotionSource, m_RootMotionMode),
    EZ_MEMBER_PROPERTY("ConstantRootMotion", m_vConstantRootMotion),
    EZ_MEMBER_PROPERTY("RootMotionDistance", m_fConstantRootMotionLength),
    EZ_MEMBER_PROPERTY("AdjustScale", m_fAnimationPositionScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f), new ezGroupAttribute("Adjustments")),
    EZ_ARRAY_MEMBER_PROPERTY("AvailableClips", m_AvailableClips)->AddAttributes(new ezReadOnlyAttribute, new ezTemporaryAttribute(), new ezContainerAttribute(false, false, false), new ezGroupAttribute("Infos")),
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

  const bool bAdditive = e.m_pObject->GetTypeAccessor().GetValue("Additive").ConvertTo<bool>();
  props["AdditiveReference"].m_Visibility = bAdditive ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

  const ezInt64 motionType = e.m_pObject->GetTypeAccessor().GetValue("RootMotion").ConvertTo<ezInt64>();
  props["ConstantRootMotion"].m_Visibility = ezPropertyUiState::Invisible;
  props["RootMotionDistance"].m_Visibility = ezPropertyUiState::Invisible;

  switch (motionType)
  {
    case ezRootMotionSource::Constant:
      props["ConstantRootMotion"].m_Visibility = ezPropertyUiState::Default;
      props["RootMotionDistance"].m_Visibility = ezPropertyUiState::Default;
      break;

    default:
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
  opt.m_AdditiveReference = (pProp->m_AdditiveReference == ezAdditiveAnimationReference::FirstKeyFrame) ? ezModelImporter2::AdditiveReference::FirstKeyFrame : ezModelImporter2::AdditiveReference::LastKeyFrame;
  opt.m_sAnimationToImport = pProp->m_sAnimationClipToExtract;
  opt.m_uiFirstAnimKeyframe = pProp->m_uiFirstFrame;
  opt.m_uiNumAnimKeyframes = pProp->m_uiNumFrames;
  opt.m_fAnimationPositionScale = pProp->m_fAnimationPositionScale;

  const ezResult res = pImporter->Import(opt);

  if (res.Succeeded())
  {
    if (pProp->m_RootMotionMode == ezRootMotionSource::Constant)
    {
      desc.m_vConstantRootMotion = pProp->m_vConstantRootMotion;

      if (pProp->m_fConstantRootMotionLength > 0.0f)
      {
        desc.m_vConstantRootMotion.SetLength(pProp->m_fConstantRootMotionLength, 0.01f).IgnoreResult();
      }
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

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  if (sMode == "AnimationClipImport_Single")
  {
    if (ezOSFile::ExistsFile(sOutFile))
    {
      ezLog::Info("Skipping animation clip import, file has been imported before: '{}'", sOutFile);
      return ezStatus(EZ_SUCCESS);
    }

    // skip the dialog below, if nothing will be imported anyway
  }

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

    ezLog::Success("Imported animation clip: '{}'", sOutFile);

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
      ezPathUtils::MakeValidFilename(clip, '-', sFilename);
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);

      if (ezOSFile::ExistsFile(sOutFile2))
      {
        ezLog::Info("Skipping animation clip import, file has been imported before: '{}'", sOutFile2);
        continue;
      }

      ezDocument* pDoc = pApp->CreateDocument(sOutFile2, ezDocumentFlags::None);
      if (pDoc == nullptr)
        return ezStatus("Could not create target document");

      out_generatedDocuments.PushBack(pDoc);

      ezAnimationClipAssetDocument* pAssetDoc = ezDynamicCast<ezAnimationClipAssetDocument*>(pDoc);

      auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
      accessor.SetValue("File", sInputFileRel.GetView());
      accessor.SetValue("UseAnimationClip", clip);
      accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

      ezLog::Success("Imported animation clip: '{}'", sOutFile2);
    }

    return ezStatus(EZ_SUCCESS);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezStatus(EZ_FAILURE);
}
