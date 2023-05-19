#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

using namespace AE_NS_FOUNDATION;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocument, 4, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocument::ezKrautTreeAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezKrautTreeAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

//////////////////////////////////////////////////////////////////////////

void CopyConfig(Kraut::SpawnNodeDesc& node, const ezKrautAssetBranchType& bt, ezDynamicArray<ezKrautMaterialDescriptor>& materials, ezKrautBranchType branchType);

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
      ref_sOut.Format("Trunk {}", (int)branchType - (int)ezKrautBranchType::Trunk1 + 1);
      break;

    case ezKrautBranchType::MainBranches1:
    case ezKrautBranchType::MainBranches2:
    case ezKrautBranchType::MainBranches3:
      ref_sOut.Format("Branch {}", (int)branchType - (int)ezKrautBranchType::MainBranches1 + 1);
      break;

    case ezKrautBranchType::SubBranches1:
    case ezKrautBranchType::SubBranches2:
    case ezKrautBranchType::SubBranches3:
      ref_sOut.Format("Twig {}", (int)branchType - (int)ezKrautBranchType::SubBranches1 + 1);
      break;

    case ezKrautBranchType::Twigs1:
    case ezKrautBranchType::Twigs2:
    case ezKrautBranchType::Twigs3:
      ref_sOut.Format("Twigy {}", (int)branchType - (int)ezKrautBranchType::Twigs1 + 1);
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


ezStatus ezKrautTreeAssetDocument::WriteKrautAsset(ezStreamWriter& stream) const
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
          CopyConfig(ts.m_BranchTypes[n], *pBts[n], desc.m_Materials, (ezKrautBranchType)n);
        }
      }

      if (n % 3 == 2)
        iBaseBranch += 3;
    }

    for (ezUInt32 n = 0; n < 5; ++n)
    {
      desc.m_LodDesc[0].m_Mode = Kraut::LodMode::Disabled;
    }

    for (ezUInt32 n = 0; n < 1; ++n)
    {
      desc.m_LodDesc[n].m_Mode = Kraut::LodMode::Full;
      desc.m_LodDesc[n].m_fTipDetail = 0.04f;
      desc.m_LodDesc[n].m_fCurvatureThreshold = 5.0f;
      desc.m_LodDesc[n].m_fThicknessThreshold = 0.2f;
      desc.m_LodDesc[n].m_fVertexRingDetail = 0.2f;

      desc.m_LodDesc[n].m_iMaxFrondDetail = 32;
      desc.m_LodDesc[n].m_iFrondDetailReduction = 0;
      desc.m_LodDesc[n].m_uiLodDistance = 0;
      desc.m_LodDesc[n].m_BranchSpikeTipMode = Kraut::BranchSpikeTipMode::FullDetail;

      desc.m_LodDesc[n].m_fCurvatureThreshold = 2.0f;
      desc.m_LodDesc[n].m_fThicknessThreshold = 5.0f / 100.0f;
      desc.m_LodDesc[n].m_fTipDetail = 0.04f;
      desc.m_LodDesc[n].m_fVertexRingDetail = 0.2f;
      desc.m_LodDesc[n].m_uiLodDistance = 10;
    }
  }

  //AssignMaterials(desc, pProp);

  // write the output data
  {
    desc.m_sSurfaceResource = pProp->m_sSurface;
    desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    desc.m_fUniformScaling = 1.0f;   // TODO pProp->m_fUniformScaling;
    desc.m_fLodDistanceScale = 1.0f; // TODO pProp->m_fLodDistanceScale;
    desc.m_GoodRandomSeeds = pProp->m_GoodRandomSeeds;
    desc.m_uiDefaultDisplaySeed = pProp->m_uiRandomSeedForDisplay;
    desc.m_fTreeStiffness = pProp->m_fTreeStiffness;

    if (desc.Serialize(stream).Failed())
    {
      return ezStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezKrautTreeAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  //ezKrautTreeAssetProperties* pProp = GetProperties();

  //if (!ezPathUtils::HasExtension(pProp->m_sKrautFile, ".tree"))
  //  return ezStatus("Unsupported file format");

  //ezKrautGeneratorResourceDescriptor desc;

  // read the input data
  //{
  //  ezFileReader krautFile;
  //  if (krautFile.Open(pProp->m_sKrautFile).Failed())
  //    return ezStatus(ezFmt("Could not open Kraut file '{0}'", pProp->m_sKrautFile));

  //  KrautStreamIn kstream;
  //  kstream.m_pStream = &krautFile;

  //  ezUInt32 uiKrautEditorVersion = 0;
  //  krautFile >> uiKrautEditorVersion;

  //  Kraut::Deserializer des;
  //  des.m_pTreeStructure = &desc.m_TreeStructureDesc;
  //  des.m_LODs[0] = &desc.m_LodDesc[0];
  //  des.m_LODs[1] = &desc.m_LodDesc[1];
  //  des.m_LODs[2] = &desc.m_LodDesc[2];
  //  des.m_LODs[3] = &desc.m_LodDesc[3];
  //  des.m_LODs[4] = &desc.m_LodDesc[4];

  //  if (!des.Deserialize(kstream))
  //  {
  //    return ezStatus(ezFmt("Reading the Kraut file failed: '{}'", pProp->m_sKrautFile));
  //  }
  //}

  // find materials
  {
    //AssignMaterials(desc, pProp);
  }

  // write the output data
  {
    //desc.m_sSurfaceResource = pProp->m_sSurface;
    //desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    //desc.m_fUniformScaling = pProp->m_fUniformScaling;
    //desc.m_fLodDistanceScale = pProp->m_fLodDistanceScale;
    //desc.m_GoodRandomSeeds = pProp->m_GoodRandomSeeds;
    //desc.m_uiDefaultDisplaySeed = pProp->m_uiRandomSeedForDisplay;
    //desc.m_fTreeStiffness = pProp->m_fTreeStiffness;

    if (WriteKrautAsset(stream).Failed())
    //if (desc.Serialize(stream).Failed())
    {
      return ezStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  //SyncBackAssetProperties(pProp, desc);

  return ezStatus(EZ_SUCCESS);
}

void ezKrautTreeAssetDocument::AssignMaterials(ezKrautGeneratorResourceDescriptor& desc, const ezKrautTreeAssetProperties* pProp) const
{
  desc.m_Materials.Clear();

  ezStringBuilder materialLabel;

  for (ezUInt32 bt = 0; bt < Kraut::BranchType::ENUM_COUNT; ++bt)
  {
    const auto& type = desc.m_TreeStructureDesc.m_BranchTypes[bt];

    if (!type.m_bUsed)
      continue;

    for (ezUInt32 gt = 0; gt < Kraut::BranchGeometryType::ENUM_COUNT; ++gt)
    {
      if (!type.m_bEnable[gt])
        continue;

      auto& m = desc.m_Materials.ExpandAndGetRef();

      m.m_MaterialType = static_cast<ezKrautMaterialType>((int)ezKrautMaterialType::Branch + gt);
      m.m_BranchType = static_cast<ezKrautBranchType>((int)ezKrautBranchType::Trunk1 + bt);

      GetMaterialLabel(materialLabel, m.m_BranchType, m.m_MaterialType);

      // find the matching material from the user input (don't want to guess an index, in case the list size changed)
      for (const auto& mat : pProp->m_Materials)
      {
        if (mat.m_sLabel == materialLabel)
        {
          m.m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(mat.m_sMaterial);
          break;
        }
      }
    }
  }
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

void ezKrautTreeAssetDocumentGenerator::GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const
{
  ezStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension("ezKrautTreeAsset");

  {
    ezAssetDocumentGenerator::Info& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "KrautTreeImport.Tree";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Kraut_Tree.png";
  }
}

ezStatus ezKrautTreeAssetDocumentGenerator::Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezKrautTreeAssetDocument* pAssetDoc = ezDynamicCast<ezKrautTreeAssetDocument*>(out_pGeneratedDocument);

  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", sDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
