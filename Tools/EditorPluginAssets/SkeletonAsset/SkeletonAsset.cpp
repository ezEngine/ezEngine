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

  if (outScene->m_pSkeleton == nullptr || outScene->m_pSkeleton->GetBoneCount() == 0)
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

  const float fScale = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);
  const ezMat3 mTransformation = ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, fScale);
  const ezMat3 mTransformRotations = ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir);

  ezSharedPtr<Scene> scene;
  EZ_SUCCEED_OR_RETURN(ImportSkeleton(pProp->m_sAnimationFile, scene));

  ezEditableSkeleton* pNewSkeleton = EZ_DEFAULT_NEW(ezEditableSkeleton);

  const ezUInt32 numBones = scene->m_pSkeleton->GetBoneCount();

  ezDynamicArray<ezEditableSkeletonBone*> allBones;
  allBones.SetCountUninitialized(numBones);

  ezSet<ezString> boneNames;
  ezStringBuilder tmp;

  for (ezUInt32 b = 0; b < numBones; ++b)
  {
    const ezMat4 mBoneTransform = scene->m_pSkeleton->GetBone(b).GetBindPoseLocalTransform();

    allBones[b] = EZ_DEFAULT_NEW(ezEditableSkeletonBone);
    allBones[b]->m_sName = scene->m_pSkeleton->GetBone(b).GetName();
    allBones[b]->m_Transform.SetFromMat4(mBoneTransform);
    allBones[b]->m_Transform.m_vScale.Set(1.0f);

    allBones[b]->m_fLength = 0.1f;
    allBones[b]->m_fThickness = 0.01f;
    allBones[b]->m_fWidth = 0.01f;
    allBones[b]->m_Geometry = ezSkeletonBoneGeometryType::None;

    if (boneNames.Contains(allBones[b]->m_sName))
    {
      tmp = allBones[b]->m_sName.GetData();
      tmp.AppendFormat("_bone{0}", b);
      allBones[b]->m_sName.Assign(tmp.GetData());
    }

    boneNames.Insert(allBones[b]->m_sName.GetString());

    if (scene->m_pSkeleton->GetBone(b).IsRootBone())
    {
      allBones[b]->m_Transform.m_vPosition = mTransformation * allBones[b]->m_Transform.m_vPosition;
      allBones[b]->m_Transform.m_qRotation.SetFromMat3(mTransformRotations * allBones[b]->m_Transform.m_qRotation.GetAsMat3());

      pNewSkeleton->m_Children.PushBack(allBones[b]);
    }
    else
    {
      allBones[b]->m_Transform.m_vPosition = fScale * allBones[b]->m_Transform.m_vPosition;

      ezUInt32 parentIdx = scene->m_pSkeleton->GetBone(b).GetParentIndex();
      EZ_ASSERT_DEBUG(parentIdx < b, "Invalid parent bone index");
      allBones[parentIdx]->m_Children.PushBack(allBones[b]);
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
  ezMap<ezString, const ezEditableSkeletonBone*> prevBones;

  // map all old bones by name
  {
    auto TraverseBones = [&prevBones](const auto& self, ezEditableSkeletonBone* pBone) -> void {
      prevBones[pBone->GetName()] = pBone;

      for (ezEditableSkeletonBone* pChild : pBone->m_Children)
      {
        self(self, pChild);
      }
    };

    for (ezEditableSkeletonBone* pChild : pOldSkeleton->m_Children)
    {
      TraverseBones(TraverseBones, pChild);
    }
  }

  // copy old properties to new skeleton
  {
    auto TraverseBones = [&prevBones](const auto& self, ezEditableSkeletonBone* pBone) -> void {
      auto it = prevBones.Find(pBone->GetName());
      if (it.IsValid())
      {
        pBone->CopyPropertiesFrom(it.Value());
      }

      for (ezEditableSkeletonBone* pChild : pBone->m_Children)
      {
        self(self, pChild);
      }
    };

    for (ezEditableSkeletonBone* pChild : pNewSkeleton->m_Children)
    {
      TraverseBones(TraverseBones, pChild);
    }
  }

  // get rid of all old bones
  pOldSkeleton->ClearBones();

  // move the new top level bones over to our own skeleton
  pOldSkeleton->m_Children = pNewSkeleton->m_Children;
  pNewSkeleton->m_Children.Clear(); // prevent this skeleton from deallocating the bones

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
