#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/Progress.h>
#include <Foundation/Math/Random.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSkeletonAssetDocument::ezSkeletonAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezEditableSkeleton>(szDocumentPath, true)
{
}

ezSkeletonAssetDocument::~ezSkeletonAssetDocument() = default;

ezStatus ezSkeletonAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);


  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Skeleton");

  // TODO: Read FBX, extract data, store structure (bone names, hierarchy) in a new ezEditableSkeleton
  ezEditableSkeleton* pNewSkeleton = EZ_DEFAULT_NEW(ezEditableSkeleton);

  // sets up a dummy skeleton, remove this code
  {
    pNewSkeleton->ClearBones();

    ezStringBuilder name;
    ezRandom r;
    r.InitializeFromCurrentTime();

    ezEditableSkeletonBone* pCur;

    if (pNewSkeleton->m_Children.IsEmpty())
    {
      pCur = EZ_DEFAULT_NEW(ezEditableSkeletonBone);
      pNewSkeleton->m_Children.PushBack(pCur);
    }

    pCur = pNewSkeleton->m_Children[0];
    name.Format("Root", r.UInt());
    pCur->SetName(name);
    pCur->m_Transform.SetIdentity();

    if (pNewSkeleton->m_Children[0]->m_Children.IsEmpty())
    {
      pCur = EZ_DEFAULT_NEW(ezEditableSkeletonBone);
      pNewSkeleton->m_Children[0]->m_Children.PushBack(pCur);
    }

    pCur = pNewSkeleton->m_Children[0]->m_Children[0];

    name.Format("Body", r.UInt());
    pCur->SetName(name);
    pCur->m_Transform.SetIdentity();
    pCur->m_Transform.m_vPosition.Set(0, 0, 1);

    if (pNewSkeleton->m_Children[0]->m_Children[0]->m_Children.IsEmpty())
    {
      pCur = EZ_DEFAULT_NEW(ezEditableSkeletonBone);
      pNewSkeleton->m_Children[0]->m_Children[0]->m_Children.PushBack(pCur);
    }

    pCur = pNewSkeleton->m_Children[0]->m_Children[0]->m_Children[0];

    name.Format("Head", r.UInt());
    pCur->SetName(name);
    pCur->m_Transform.SetIdentity();
    pCur->m_Transform.m_vPosition.Set(0, 0, 1.7);
  }

  // synchronize the old data (collision geometry etc.) with the new hierarchy
  // this function deletes pNewSkeleton when it's done
  MergeWithNewSkeleton(pNewSkeleton);
  pNewSkeleton = nullptr;

  // merge the new data with the actual asset document
  ApplyNativePropertyChangesToObjectManager();

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
    auto TraverseBones = [&prevBones](const auto& self, ezEditableSkeletonBone* pBone)->void
    {
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
    auto TraverseBones = [&prevBones](const auto& self, ezEditableSkeletonBone* pBone)->void
    {
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
