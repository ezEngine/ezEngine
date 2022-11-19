#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocument, 9, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezTransform CalculateTransformationMatrix(const ezEditableSkeleton* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const ezBasisAxis::Enum rightDir = pProp->m_RightDir;
  const ezBasisAxis::Enum upDir = pProp->m_UpDir;
  ezBasisAxis::Enum forwardDir = ezBasisAxis::GetOrthogonalAxis(rightDir, upDir, !pProp->m_bFlipForwardDir);

  ezTransform t;
  t.SetIdentity();
  t.m_vScale.Set(us);

  // prevent mirroring in the rotation matrix, because we can't generate a quaternion from that
  if (!pProp->m_bFlipForwardDir)
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
  t.m_qRotation.SetFromMat3(rot);

  return t;
}

ezSkeletonAssetDocument::ezSkeletonAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezEditableSkeleton>(szDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezSkeletonAssetDocument::~ezSkeletonAssetDocument() = default;

void ezSkeletonAssetDocument::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezEditableSkeletonBoneShape>())
  {
    auto& props = *e.m_pPropertyStates;

    const ezSkeletonJointGeometryType::Enum geomType = (ezSkeletonJointGeometryType::Enum)e.m_pObject->GetTypeAccessor().GetValue("Geometry").ConvertTo<ezInt32>();

    const bool overrideName = e.m_pObject->GetTypeAccessor().GetValue("OverrideName").ConvertTo<bool>();
    const bool overrideSurface = e.m_pObject->GetTypeAccessor().GetValue("OverrideSurface").ConvertTo<bool>();
    const bool overrideCollisionLayer = e.m_pObject->GetTypeAccessor().GetValue("OverrideCollisionLayer").ConvertTo<bool>();

    props["Offset"].m_Visibility = ezPropertyUiState::Invisible;
    props["Rotation"].m_Visibility = ezPropertyUiState::Invisible;
    props["Length"].m_Visibility = ezPropertyUiState::Invisible;
    props["Width"].m_Visibility = ezPropertyUiState::Invisible;
    props["Thickness"].m_Visibility = ezPropertyUiState::Invisible;
    props["Name"].m_Visibility = ezPropertyUiState::Invisible;
    props["Surface"].m_Visibility = ezPropertyUiState::Invisible;
    props["CollisionLayer"].m_Visibility = ezPropertyUiState::Invisible;
    props["OverrideName"].m_Visibility = ezPropertyUiState::Invisible;
    props["OverrideSurface"].m_Visibility = ezPropertyUiState::Invisible;
    props["OverrideCollisionLayer"].m_Visibility = ezPropertyUiState::Invisible;

    if (geomType == ezSkeletonJointGeometryType::None)
      return;

    props["Length"].m_sNewLabelText = "Length";
    props["Width"].m_sNewLabelText = "Width";
    props["Thickness"].m_sNewLabelText = "Thickness";

    props["Offset"].m_Visibility = ezPropertyUiState::Default;
    props["Rotation"].m_Visibility = ezPropertyUiState::Default;
    props["OverrideName"].m_Visibility = ezPropertyUiState::Default;
    props["OverrideSurface"].m_Visibility = ezPropertyUiState::Default;
    props["OverrideCollisionLayer"].m_Visibility = ezPropertyUiState::Default;

    props["Name"].m_Visibility = overrideName ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["Surface"].m_Visibility = overrideSurface ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["CollisionLayer"].m_Visibility = overrideCollisionLayer ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

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
  }
}

ezStatus ezSkeletonAssetDocument::WriteResource(ezStreamWriter& stream) const
{
  auto pProp = GetProperties(); // ApplyNativePropertyChangesToObjectManager destroys pProp

  ezSkeletonResourceDescriptor desc;
  desc.m_RootTransform = CalculateTransformationMatrix(pProp);
  pProp->FillResourceDescriptor(desc);

  EZ_SUCCEED_OR_RETURN(desc.Serialize(stream));

  return ezStatus(EZ_SUCCESS);
}

void ezSkeletonAssetDocument::SetRenderBones(bool enable)
{
  if (m_bRenderBones == enable)
    return;

  m_bRenderBones = enable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderColliders(bool enable)
{
  if (m_bRenderColliders == enable)
    return;

  m_bRenderColliders = enable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderJoints(bool enable)
{
  if (m_bRenderJoints == enable)
    return;

  m_bRenderJoints = enable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderSwingLimits(bool enable)
{
  if (m_bRenderSwingLimits == enable)
    return;

  m_bRenderSwingLimits = enable;

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::RenderStateChanged;
  m_Events.Broadcast(e);
}

void ezSkeletonAssetDocument::SetRenderTwistLimits(bool enable)
{
  if (m_bRenderTwistLimits == enable)
    return;

  m_bRenderTwistLimits = enable;

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

  auto Traverse = [&](ezEditableSkeletonJoint* pJoint, const char* szParent, auto Recurse) -> void {
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
      Recurse(pChild, pJoint->GetName(), Recurse);
    }
  };

  for (auto ptr : desc->m_Children)
  {
    Traverse(ptr, "", Traverse);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezTransformStatus ezSkeletonAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    EZ_SCOPE_EXIT(m_bIsTransforming = false);

    ezProgressRange range("Transforming Asset", 3, false);

    ezEditableSkeleton* pProp = GetProperties();

    ezStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (!sAbsFilename.IsEmpty())
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
      MergeWithNewSkeleton(newSkeleton);

      // merge the new data with the actual asset document
      ApplyNativePropertyChangesToObjectManager(true);
    }

    range.BeginNextStep("Writing Result");

    EZ_SUCCEED_OR_RETURN(WriteResource(stream));
  }

  ezSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezSkeletonAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void ezSkeletonAssetDocument::MergeWithNewSkeleton(ezEditableSkeleton& newSkeleton)
{
  ezEditableSkeleton* pOldSkeleton = GetProperties();
  ezMap<ezString, const ezEditableSkeletonJoint*> prevJoints;

  // map all old joints by name
  {
    auto TraverseJoints = [&prevJoints](const auto& self, ezEditableSkeletonJoint* pJoint) -> void {
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
    auto TraverseJoints = [&prevJoints](const auto& self, ezEditableSkeletonJoint* pJoint, const ezTransform& tRoot, ezTransform origin) -> void {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      // use the parent rotation as the gizmo base rotation
      ezMat4 modelTransform, fullTransform;
      modelTransform = origin.GetAsMat4();
      ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(tRoot.GetAsMat4(), modelTransform, fullTransform, pJoint->m_qGizmoOffsetRotationRO);

      origin.SetGlobalTransform(origin, pJoint->m_LocalTransform);
      pJoint->m_vGizmoOffsetPositionRO = tRoot.TransformPosition(origin.m_vPosition);

      for (ezEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild, tRoot, origin);
      }
    };

    for (ezEditableSkeletonJoint* pChild : newSkeleton.m_Children)
    {
      TraverseJoints(TraverseJoints, pChild, CalculateTransformationMatrix(pOldSkeleton), ezTransform::IdentityTransform());
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = newSkeleton.m_Children;
  newSkeleton.m_Children.Clear(); // prevent this skeleton from deallocating the joints
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

void ezSkeletonAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::Undecided;
    info.m_sName = "SkeletonImport";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Skeleton.png";
  }
}

ezStatus ezSkeletonAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezSkeletonAssetDocument* pAssetDoc = ezDynamicCast<ezSkeletonAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezSkeletonAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
