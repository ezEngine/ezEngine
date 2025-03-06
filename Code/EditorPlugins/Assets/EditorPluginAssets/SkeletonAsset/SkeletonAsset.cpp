#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocument, 11, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezTransform CalculateTransformationMatrix(const ezEditableSkeleton* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  auto rightDir = ezMeshImportTransform::GetRightDir(pProp->m_ImportTransform, pProp->m_RightDir);
  auto upDir = ezMeshImportTransform::GetUpDir(pProp->m_ImportTransform, pProp->m_UpDir);
  auto flipFwd = ezMeshImportTransform::GetFlipForward(pProp->m_ImportTransform, pProp->m_bFlipForwardDir);

  ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(rightDir, upDir, !flipFwd);

  ezTransform t;
  t.SetIdentity();
  t.m_vScale.Set(us);

  // prevent mirroring in the rotation matrix, because we can't generate a quaternion from that
  if (!flipFwd)
  {
    switch (forwardDir)
    {
      case ezBasisAxis::PositiveX:
        forwardDir = ezBasisAxis::NegativeX;
        t.m_vScale.x *= -1;
        break;
      case ezBasisAxis::PositiveY:
        forwardDir = ezBasisAxis::NegativeY;
        t.m_vScale.y *= -1;
        break;
      case ezBasisAxis::PositiveZ:
        forwardDir = ezBasisAxis::NegativeZ;
        t.m_vScale.z *= -1;
        break;
      case ezBasisAxis::NegativeX:
        forwardDir = ezBasisAxis::PositiveX;
        t.m_vScale.x *= -1;
        break;
      case ezBasisAxis::NegativeY:
        forwardDir = ezBasisAxis::PositiveY;
        t.m_vScale.y *= -1;
        break;
      case ezBasisAxis::NegativeZ:
        forwardDir = ezBasisAxis::PositiveZ;
        t.m_vScale.z *= -1;
        break;
    }
  }

  ezMat3 rot = ezBasisAxis::CalculateTransformationMatrix(forwardDir, rightDir, upDir, 1.0f);
  t.m_qRotation = ezQuat::MakeFromMat3(rot);

  return t;
}

ezSkeletonAssetDocument::ezSkeletonAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezEditableSkeleton>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezSkeletonAssetDocument::~ezSkeletonAssetDocument() = default;

void ezSkeletonAssetDocument::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezEditableSkeleton>())
  {
    auto& props = *e.m_pPropertyStates;

    const ezInt64 importTransform = e.m_pObject->GetTypeAccessor().GetValue("ImportTransform").ConvertTo<ezInt64>();
    const bool bCustomTransform = importTransform == 127;
    props["RightDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["UpDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["FlipForwardDir"].m_Visibility = bCustomTransform ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezEditableSkeletonJoint>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool overrideSurface = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();
    const ezSkeletonJointType::Enum jointType = (ezSkeletonJointType::Enum)e.m_pObject->GetTypeAccessor().GetValue("JointType").ConvertTo<ezInt32>();

    const bool bHasStiffness = jointType == ezSkeletonJointType::SwingTwist;
    const bool bHasSwing = jointType == ezSkeletonJointType::SwingTwist;
    const bool bHasTwist = jointType == ezSkeletonJointType::SwingTwist;

    props["CollisionLayer"].m_Visibility = overrideCollisionLayer ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["Surface"].m_Visibility = overrideSurface ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    props["LocalRotation"].m_Visibility = bHasStiffness ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["Stiffness"].m_Visibility = bHasStiffness ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SwingLimitY"].m_Visibility = bHasSwing ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SwingLimitZ"].m_Visibility = bHasSwing ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["TwistLimitHalfAngle"].m_Visibility = bHasTwist ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["TwistLimitCenterAngle"].m_Visibility = bHasTwist ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    return;
  }

  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezEditableSkeletonBoneShape>())
  {
    auto& props = *e.m_pPropertyStates;

    const ezSkeletonJointGeometryType::Enum geomType = (ezSkeletonJointGeometryType::Enum)e.m_pObject->GetTypeAccessor().GetValue("Geometry").ConvertTo<ezInt32>();

    props["Offset"].m_Visibility = ezPropertyUiState::Invisible;
    props["Rotation"].m_Visibility = ezPropertyUiState::Invisible;
    props["Length"].m_Visibility = ezPropertyUiState::Invisible;
    props["Width"].m_Visibility = ezPropertyUiState::Invisible;
    props["Thickness"].m_Visibility = ezPropertyUiState::Invisible;

    if (geomType == ezSkeletonJointGeometryType::None)
      return;

    props["Length"].m_sNewLabelText = "Length";
    props["Width"].m_sNewLabelText = "Width";
    props["Thickness"].m_sNewLabelText = "Thickness";

    props["Offset"].m_Visibility = ezPropertyUiState::Default;
    props["Rotation"].m_Visibility = ezPropertyUiState::Default;

    if (geomType == ezSkeletonJointGeometryType::Box)
    {
      props["Length"].m_Visibility = ezPropertyUiState::Default;
      props["Width"].m_Visibility = ezPropertyUiState::Default;
      props["Thickness"].m_Visibility = ezPropertyUiState::Default;
    }
    else if (geomType == ezSkeletonJointGeometryType::Sphere)
    {
      props["Thickness"].m_Visibility = ezPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }
    else if (geomType == ezSkeletonJointGeometryType::Capsule)
    {
      props["Length"].m_Visibility = ezPropertyUiState::Default;

      props["Thickness"].m_Visibility = ezPropertyUiState::Default;
      props["Thickness"].m_sNewLabelText = "Radius";
    }

    return;
  }
}

ezStatus ezSkeletonAssetDocument::WriteResource(ezStreamWriter& inout_stream, const ezEditableSkeleton& skeleton) const
{
  ezSkeletonResourceDescriptor desc;
  desc.m_RootTransform = CalculateTransformationMatrix(&skeleton);
  skeleton.FillResourceDescriptor(desc);

  EZ_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return ezStatus(EZ_SUCCESS);
}

void ezSkeletonAssetDocument::SetRenderBones(bool bEnable)
{
  if (m_bRenderBones == bEnable)
    return;

  m_bRenderBones = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderColliders(bool bEnable)
{
  if (m_bRenderColliders == bEnable)
    return;

  m_bRenderColliders = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderJoints(bool bEnable)
{
  if (m_bRenderJoints == bEnable)
    return;

  m_bRenderJoints = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderSwingLimits(bool bEnable)
{
  if (m_bRenderSwingLimits == bEnable)
    return;

  m_bRenderSwingLimits = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderTwistLimits(bool bEnable)
{
  if (m_bRenderTwistLimits == bEnable)
    return;

  m_bRenderTwistLimits = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderPreviewMesh(bool bEnable)
{
  if (m_bRenderPreviewMesh == bEnable)
    return;

  m_bRenderPreviewMesh = bEnable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // expose all the bones as parameters
  // such that we can create components that modify these bones

  auto* desc = GetProperties();
  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);


  {
    ezExposedBone bone;
    bone.m_sName = "<root-transform>";
    bone.m_Transform = CalculateTransformationMatrix(desc);

    ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
    param->m_sName = "<root-transform>";
    param->m_DefaultValue.CopyTypedObject(&bone, ezGetStaticRTTI<ezExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);
  }

  auto Traverse = [&](ezEditableSkeletonJoint* pJoint, const char* szParent, auto recurse) -> void
  {
    ezExposedBone bone;
    bone.m_sName = pJoint->GetName();
    bone.m_sParent = szParent;
    bone.m_Transform = pJoint->m_LocalTransform;

    ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
    param->m_sName = pJoint->GetName();
    param->m_DefaultValue.CopyTypedObject(&bone, ezGetStaticRTTI<ezExposedBone>());

    pExposedParams->m_Parameters.PushBack(param);

    for (auto pChild : pJoint->m_Children)
    {
      recurse(pChild, pJoint->GetName(), recurse);
    }
  };

  for (auto ptr : desc->m_Children)
  {
    Traverse(ptr, "", Traverse);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezTransformStatus ezSkeletonAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    EZ_SCOPE_EXIT(m_bIsTransforming = false);

    ezProgressRange range("Transforming Asset", 3, false);

    ezEditableSkeleton* pProp = GetProperties();

    ezStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (sAbsFilename.IsEmpty())
    {
      range.BeginNextStep("Writing Result");
      EZ_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties()));
    }
    else
    {
      if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
      {
        return ezStatus(ezFmt("Couldn't make path absolute: '{0};", sAbsFilename));
      }

      ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(sAbsFilename);
      if (pImporter == nullptr)
        return ezStatus("No known importer for this file type.");

      range.BeginNextStep("Importing Source File");

      ezEditableSkeleton newSkeleton;

      ezModelImporter2::ImportOptions opt;
      opt.m_sSourceFile = sAbsFilename;
      opt.m_pSkeletonOutput = &newSkeleton;

      if (pImporter->Import(opt).Failed())
        return ezStatus("Model importer was unable to read this asset.");

      range.BeginNextStep("Importing Skeleton Data");

      // synchronize the old data (collision geometry etc.) with the new hierarchy
      const ezEditableSkeleton* pFinalSkeleton = MergeWithNewSkeleton(newSkeleton);

      range.BeginNextStep("Writing Result");
      EZ_SUCCEED_OR_RETURN(WriteResource(stream, *pFinalSkeleton));

      // merge the new data with the actual asset document
      ApplyNativePropertyChangesToObjectManager(true);
    }
  }

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails wouldn't look as desired, for assets transformed in the background
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

const ezEditableSkeleton* ezSkeletonAssetDocument::MergeWithNewSkeleton(ezEditableSkeleton& newSkeleton)
{
  ezEditableSkeleton* pOldSkeleton = GetProperties();
  ezMap<ezString, const ezEditableSkeletonJoint*> prevJoints;

  // map all old joints by name
  {
    auto TraverseJoints = [&prevJoints](const auto& self, ezEditableSkeletonJoint* pJoint) -> void
    {
      prevJoints[pJoint->GetName()] = pJoint;

      for (ezEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild);
      }
    };

    for (ezEditableSkeletonJoint* pChild : pOldSkeleton->m_Children)
    {
      TraverseJoints(TraverseJoints, pChild);
    }
  }

  // copy old properties to new skeleton
  {
    auto TraverseJoints = [&prevJoints](const auto& self, ezEditableSkeletonJoint* pJoint, const ezTransform& root, ezTransform origin) -> void
    {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      // use the parent rotation as the gizmo base rotation
      ezMat4 modelTransform, fullTransform;
      modelTransform = origin.GetAsMat4();
      ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(root.GetAsMat4(), modelTransform, fullTransform, pJoint->m_qGizmoOffsetRotationRO);

      origin = ezTransform::MakeGlobalTransform(origin, pJoint->m_LocalTransform);
      pJoint->m_vGizmoOffsetPositionRO = root.TransformPosition(origin.m_vPosition);

      for (ezEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild, root, origin);
      }
    };

    for (ezEditableSkeletonJoint* pChild : newSkeleton.m_Children)
    {
      TraverseJoints(TraverseJoints, pChild, CalculateTransformationMatrix(pOldSkeleton), ezTransform::MakeIdentity());
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = newSkeleton.m_Children;
  newSkeleton.m_Children.Clear(); // prevent this skeleton from deallocating the joints

  return pOldSkeleton;
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezSkeletonAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSkeletonAssetDocumentGenerator::ezSkeletonAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezSkeletonAssetDocumentGenerator::~ezSkeletonAssetDocumentGenerator() = default;

void ezSkeletonAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::Undecided;
    info.m_sName = "SkeletonImport";
    info.m_sIcon = ":/AssetIcons/Skeleton.svg";
  }
}

ezStatus ezSkeletonAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
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

  ezSkeletonAssetDocument* pAssetDoc = ezDynamicCast<ezSkeletonAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", sInputFileRel.GetView());

  return ezStatus(EZ_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezEditableSkeleton_1_2 : public ezGraphPatch
{
public:
  ezEditableSkeleton_1_2()
    : ezGraphPatch("ezEditableSkeleton", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("ImportTransform", 127);
  }
};

ezEditableSkeleton_1_2 g_ezEditableSkeleton_1_2;
