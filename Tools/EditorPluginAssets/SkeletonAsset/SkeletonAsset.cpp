#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/Scene.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSkeletonAssetDocument::ezSkeletonAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezEditableSkeleton>(szDocumentPath, true)
{
}

ezSkeletonAssetDocument::~ezSkeletonAssetDocument() = default;

using namespace ezModelImporter;

static ezStatus ImportSkeleton(const char* filename, ezSharedPtr<ezModelImporter::Scene>& outScene)
{
  ezStringBuilder sAbsFilename = filename;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  outScene = Importer::GetSingleton()->ImportScene(sAbsFilename, ImportFlags::Skeleton);

  if (outScene == nullptr)
    return ezStatus(ezFmt("Input file '{0}' could not be imported", filename));

  if (outScene->m_Skeleton.GetJointCount() == 0)
    return ezStatus("Mesh does not contain skeleton information");

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSkeletonAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                         const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Skeleton");

  ezEditableSkeleton* pProp = GetProperties();

  const float fScale = 1.0f;
  // ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);
  ezMat3 mTransformation = ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, fScale);
  ezMat3 mTransformRotations = ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir);

  mTransformation.SetIdentity();
  mTransformRotations.SetIdentity();

  ezSharedPtr<Scene> scene;
  EZ_SUCCEED_OR_RETURN(ImportSkeleton(pProp->m_sAnimationFile, scene));

  ezEditableSkeleton* pNewSkeleton = EZ_DEFAULT_NEW(ezEditableSkeleton);

  const ezUInt32 numJoints = scene->m_Skeleton.GetJointCount();

  ezDynamicArray<ezEditableSkeletonJoint*> allJoints;
  allJoints.SetCountUninitialized(numJoints);

  ezSet<ezString> jointNames;
  ezStringBuilder tmp;

  for (ezUInt32 b = 0; b < numJoints; ++b)
  {
    const ezTransform mJointTransform = scene->m_Skeleton.GetJointByIndex(b).GetBindPoseLocalTransform();

    allJoints[b] = EZ_DEFAULT_NEW(ezEditableSkeletonJoint);
    allJoints[b]->m_sName = scene->m_Skeleton.GetJointByIndex(b).GetName();
    allJoints[b]->m_Transform = mJointTransform;
    allJoints[b]->m_Transform.m_vScale.Set(1.0f);

    allJoints[b]->m_fLength = 0.1f;
    allJoints[b]->m_fThickness = 0.01f;
    allJoints[b]->m_fWidth = 0.01f;
    allJoints[b]->m_Geometry = ezSkeletonJointGeometryType::None;

    if (jointNames.Contains(allJoints[b]->m_sName))
    {
      tmp = allJoints[b]->m_sName.GetData();
      tmp.AppendFormat("_joint{0}", b);
      allJoints[b]->m_sName.Assign(tmp.GetData());
    }

    jointNames.Insert(allJoints[b]->m_sName.GetString());

    if (scene->m_Skeleton.GetJointByIndex(b).IsRootJoint())
    {
      allJoints[b]->m_Transform.m_vPosition = mTransformation * allJoints[b]->m_Transform.m_vPosition;
      allJoints[b]->m_Transform.m_qRotation.SetFromMat3(mTransformRotations * allJoints[b]->m_Transform.m_qRotation.GetAsMat3());

      pNewSkeleton->m_Children.PushBack(allJoints[b]);
    }
    else
    {
      allJoints[b]->m_Transform.m_vPosition = fScale * allJoints[b]->m_Transform.m_vPosition;

      ezUInt32 parentIdx = scene->m_Skeleton.GetJointByIndex(b).GetParentIndex();
      EZ_ASSERT_DEBUG(parentIdx < b, "Invalid parent joint index");
      allJoints[parentIdx]->m_Children.PushBack(allJoints[b]);
    }
  }

  // synchronize the old data (collision geometry etc.) with the new hierarchy
  // this function deletes pNewSkeleton when it's done
  MergeWithNewSkeleton(pNewSkeleton);
  pNewSkeleton = nullptr;

  // merge the new data with the actual asset document
  ApplyNativePropertyChangesToObjectManager(true);

  range.BeginNextStep("Writing Result");

  ezSkeletonResourceDescriptor desc;
  GetProperties()->FillResourceDescriptor(desc);

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSkeletonAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

void ezSkeletonAssetDocument::MergeWithNewSkeleton(ezEditableSkeleton* pNewSkeleton)
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
    auto TraverseJoints = [&prevJoints](const auto& self, ezEditableSkeletonJoint* pJoint) -> void {
      auto it = prevJoints.Find(pJoint->GetName());
      if (it.IsValid())
      {
        pJoint->CopyPropertiesFrom(it.Value());
      }

      for (ezEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        self(self, pChild);
      }
    };

    for (ezEditableSkeletonJoint* pChild : pNewSkeleton->m_Children)
    {
      TraverseJoints(TraverseJoints, pChild);
    }
  }

  // get rid of all old joints
  pOldSkeleton->ClearJoints();

  // move the new top level joints over to our own skeleton
  pOldSkeleton->m_Children = pNewSkeleton->m_Children;
  pNewSkeleton->m_Children.Clear(); // prevent this skeleton from deallocating the joints

  // there is no use for the new skeleton anymore
  EZ_DEFAULT_DELETE(pNewSkeleton);
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezSkeletonAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSkeletonAssetDocumentGenerator::ezSkeletonAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
}

ezSkeletonAssetDocumentGenerator::~ezSkeletonAssetDocumentGenerator() = default;

void ezSkeletonAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                      ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
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

ezStatus ezSkeletonAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                    ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateOrOpenDocument(true, info.m_sOutputFileAbsolute, false, false);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezSkeletonAssetDocument* pAssetDoc = ezDynamicCast<ezSkeletonAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezSkeletonAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("File", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
