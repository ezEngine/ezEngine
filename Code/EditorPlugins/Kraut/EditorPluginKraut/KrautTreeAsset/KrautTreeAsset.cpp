#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>

using namespace AE_NS_FOUNDATION;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocument, 5, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocument::ezKrautTreeAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezKrautTreeAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

void ezKrautTreeAssetDocument::SetWindStrength(ezKrautWindStrength::Enum strength)
{
  if (m_WindStrength == strength)
    return;

  m_WindStrength = strength;

  // Send message to engine to update wind
  ezSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "SetWindStrength";
  msg.m_sPayload = "";
  msg.m_PayloadValue = (ezInt32)strength;
  SendMessageToEngine(&msg);

  // Notify actions to update their checked state
  ezKrautTreeAssetEvent e;
  e.m_Type = ezKrautTreeAssetEvent::Type::WindStrengthChanged;
  m_Events.Broadcast(e);
}

void ezKrautTreeAssetDocument::SetShowFrondsLeaves(bool bShow)
{
  if (m_bShowFrondsLeaves == bShow)
    return;

  m_bShowFrondsLeaves = bShow;

  // Send message to engine to update component
  ezSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "SetShowFrondsLeaves";
  msg.m_sPayload = "";
  msg.m_PayloadValue = (ezInt32)bShow;
  SendMessageToEngine(&msg);

  // Notify actions to update their checked state
  ezKrautTreeAssetEvent e;
  e.m_Type = ezKrautTreeAssetEvent::Type::FrondsLeavesVisibilityChanged;
  m_Events.Broadcast(e);
}

//////////////////////////////////////////////////////////////////////////

void CopyKrautConfig(Kraut::SpawnNodeDesc& ref_node, const ezKrautAssetBranchType& bt, ezDynamicArray<ezKrautMaterialDescriptor>& ref_materials, ezKrautBranchType branchType);

class KrautStreamIn : public aeStreamIn
{
public:
  ezStreamReader* m_pStream = nullptr;

private:
  virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_pStream->ReadBytes(pData, uiSize); }
};

static void GetMaterialLabel(ezStringBuilder& ref_sOut, ezKrautBranchType branchType, ezKrautMaterialType materialType)
{
  ref_sOut.Clear();

  switch (branchType)
  {
    case ezKrautBranchType::Trunk1:
    case ezKrautBranchType::Trunk2:
    case ezKrautBranchType::Trunk3:
      ref_sOut.SetFormat("Trunk {}", (int)branchType - (int)ezKrautBranchType::Trunk1 + 1);
      break;

    case ezKrautBranchType::MainBranches1:
    case ezKrautBranchType::MainBranches2:
    case ezKrautBranchType::MainBranches3:
      ref_sOut.SetFormat("Branch {}", (int)branchType - (int)ezKrautBranchType::MainBranches1 + 1);
      break;

    case ezKrautBranchType::SubBranches1:
    case ezKrautBranchType::SubBranches2:
    case ezKrautBranchType::SubBranches3:
      ref_sOut.SetFormat("Twig {}", (int)branchType - (int)ezKrautBranchType::SubBranches1 + 1);
      break;

    case ezKrautBranchType::Twigs1:
    case ezKrautBranchType::Twigs2:
    case ezKrautBranchType::Twigs3:
      ref_sOut.SetFormat("Twigy {}", (int)branchType - (int)ezKrautBranchType::Twigs1 + 1);
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  switch (materialType)
  {
    case ezKrautMaterialType::Branch:
      ref_sOut.Append(" - Stem");
      break;
    case ezKrautMaterialType::Frond:
      ref_sOut.Append(" - Frond");
      break;
    case ezKrautMaterialType::Leaf:
      ref_sOut.Append(" - Leaf");
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}


ezStatus ezKrautTreeAssetDocument::WriteKrautAsset(ezStreamWriter& ref_stream) const
{
  const ezKrautTreeAssetProperties* pProp = GetProperties();

  ezKrautGeneratorResourceDescriptor desc;
  desc.m_uiDefaultDisplaySeed = 0;
  desc.m_GoodRandomSeeds.PushBack(0);

  auto& ts = desc.m_TreeStructureDesc;

  const ezKrautAssetBranchType* pBts[12] =
    {
      &pProp->m_BT_Trunk1,
      nullptr,
      nullptr,
      &pProp->m_BT_MainBranch1,
      &pProp->m_BT_MainBranch2,
      &pProp->m_BT_MainBranch3,
      &pProp->m_BT_SubBranch1,
      &pProp->m_BT_SubBranch2,
      &pProp->m_BT_SubBranch3,
      &pProp->m_BT_Twig1,
      &pProp->m_BT_Twig2,
      &pProp->m_BT_Twig3};

  {
    ezInt32 iBaseBranch = -3;
    for (ezUInt32 n = 0; n < Kraut::BranchType::ENUM_COUNT; ++n)
    {
      ts.m_BranchTypes[n].m_Type = (Kraut::BranchType::Enum)n;
      ts.m_BranchTypes[n].Reset();
      ts.m_BranchTypes[n].m_bUsed = false;
      ts.m_BranchTypes[n].m_bAllowSubType[0] = false;
      ts.m_BranchTypes[n].m_bAllowSubType[1] = false;
      ts.m_BranchTypes[n].m_bAllowSubType[2] = false;

      if (pBts[n] != nullptr)
      {
        if (iBaseBranch < 0 ||
            (ts.m_BranchTypes[iBaseBranch + 0].m_bAllowSubType[n % 3]) ||
            (ts.m_BranchTypes[iBaseBranch + 1].m_bAllowSubType[n % 3]) ||
            (ts.m_BranchTypes[iBaseBranch + 2].m_bAllowSubType[n % 3]))
        {
          ts.m_BranchTypes[n].m_bUsed = true;
          CopyKrautConfig(ts.m_BranchTypes[n], *pBts[n], desc.m_Materials, (ezKrautBranchType)n);
        }
      }

      if (n % 3 == 2)
        iBaseBranch += 3;
    }

    const ezKrautAssetLod* pLods[5] = {
      &pProp->m_Lod0,
      &pProp->m_Lod1,
      &pProp->m_Lod2,
      &pProp->m_Lod3,
      &pProp->m_Lod4};

    for (ezUInt32 n = 0; n < 5; ++n)
    {
      const ezKrautAssetLod& lod = *pLods[n];

      desc.m_LodDesc[n].m_fTipDetail = lod.m_fTipDetail;
      desc.m_LodDesc[n].m_fCurvatureThreshold = lod.m_fCurvatureThreshold;
      desc.m_LodDesc[n].m_fThicknessThreshold = lod.m_fThicknessThreshold;
      desc.m_LodDesc[n].m_fVertexRingDetail = lod.m_fVertexRingDetail;

      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Branch] = lod.m_AllowBranch.GetValue();
      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Frond] = lod.m_AllowFrond.GetValue();
      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Leaf] = lod.m_AllowLeaf.GetValue();

      desc.m_LodDesc[n].m_iMaxFrondDetail = lod.m_iMaxFrondDetail;
      desc.m_LodDesc[n].m_iFrondDetailReduction = lod.m_iFrondDetailReduction;
      desc.m_LodDesc[n].m_uiLodDistance = lod.m_uiLodDistance;
      desc.m_LodDesc[n].m_BranchSpikeTipMode = (Kraut::BranchSpikeTipMode::Enum)lod.m_BranchSpikeTipMode.GetValue();
    }
  }

  // write the output data
  {
    desc.m_sSurfaceResource = pProp->m_sSurface;
    desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    desc.m_fUniformScaling = 1.0f;   // TODO pProp->m_fUniformScaling;
    desc.m_fLodDistanceScale = 1.0f; // TODO pProp->m_fLodDistanceScale;
    desc.m_GoodRandomSeeds = pProp->m_GoodRandomSeeds;
    desc.m_uiDefaultDisplaySeed = pProp->m_uiRandomSeedForDisplay;
    desc.m_fTreeStiffness = pProp->m_fTreeStiffness;
    desc.m_fMinAmbientOcclusion = pProp->m_fMinAmbientOcclusion;

    if (desc.Serialize(ref_stream).Failed())
    {
      return ezStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezKrautTreeAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  if (WriteKrautAsset(stream).Failed())
  {
    return ezStatus("Writing KrautGenerator resource descriptor failed.");
  }

  return ezStatus(EZ_SUCCESS);
}

void ezKrautTreeAssetDocument::SyncBackAssetProperties(ezKrautTreeAssetProperties*& pProp, const ezKrautGeneratorResourceDescriptor& desc)
{
  bool bModified = pProp->m_Materials.GetCount() != desc.m_Materials.GetCount();

  pProp->m_Materials.SetCount(desc.m_Materials.GetCount());

  ezStringBuilder newLabel;

  // TODO: match up old and new materials by label name

  for (ezUInt32 m = 0; m < pProp->m_Materials.GetCount(); ++m)
  {
    auto& mat = pProp->m_Materials[m];

    GetMaterialLabel(newLabel, desc.m_Materials[m].m_BranchType, desc.m_Materials[m].m_MaterialType);

    if (newLabel != mat.m_sLabel)
    {
      mat.m_sLabel = newLabel;
      bModified = true;
    }
  }

  if (bModified)
  {
    GetObjectAccessor()->StartTransaction("Update Kraut Material Info");
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }
}

ezTransformStatus ezKrautTreeAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocumentGenerator::ezKrautTreeAssetDocumentGenerator()
{
  AddSupportedFileType("tree");
}

ezKrautTreeAssetDocumentGenerator::~ezKrautTreeAssetDocumentGenerator() = default;

void ezKrautTreeAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "KrautTreeImport.Tree";
    info.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  }
}

ezStatus ezKrautTreeAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping Kraut tree import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezKrautTreeAssetDocument* pAssetDoc = ezDynamicCast<ezKrautTreeAssetDocument*>(pDoc);

  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", sInputFileRel.GetView());

  ezLog::Success("Imported Kraut tree: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}
