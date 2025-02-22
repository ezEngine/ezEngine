#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <FmodPlugin/FmodIncludes.h>
#include <Foundation/IO/OSFile.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundBankAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class ezSimpleFmod
{
public:
  ezSimpleFmod() = default;
  ~ezSimpleFmod() { EZ_ASSERT_DEV(m_pSystem == nullptr, "FMod is not shut down"); }

  void Startup()
  {
    EZ_ASSERT_DEV(m_pSystem == nullptr, "FMod is not shut down");

    EZ_FMOD_ASSERT(FMOD::Studio::System::create(&m_pSystem));

    void* extraDriverData = nullptr;
    EZ_FMOD_ASSERT(m_pSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData));
  }

  void Shutdown()
  {
    if (m_pSystem == nullptr)
      return;

    EZ_FMOD_ASSERT(m_pSystem->unloadAll());
    EZ_FMOD_ASSERT(m_pSystem->release());

    m_pSystem = nullptr;
  }

  FMOD::Studio::System* GetSystem()
  {
    if (m_pSystem == nullptr)
      Startup();

    return m_pSystem;
  }

private:
  FMOD::Studio::System* m_pSystem = nullptr;
};


ezSoundBankAssetDocumentManager::ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Sound Bank";
  m_DocTypeDesc.m_sFileExtension = "ezSoundBankAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Sound_Bank.svg";
  m_DocTypeDesc.m_sAssetCategory = "Sound";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSoundBankAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Fmod_Bank");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinFmodSoundBank";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Sound Bank", QPixmap(":/AssetIcons/Sound_Bank.svg"));

  m_pFmod = EZ_DEFAULT_NEW(ezSimpleFmod);
}

ezSoundBankAssetDocumentManager::~ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  m_pFmod->Shutdown();
  m_pFmod.Clear();
}

void ezSoundBankAssetDocumentManager::FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezDynamicArray<ezSubAssetData>& out_subAssets) const
{
  EZ_PROFILE_SCOPE("GetSoundBankSubAssets");

  SoundBankCache& cache = m_Cache[assetInfo.m_DocumentID];
  const ezTimestamp lastTS = cache.m_LastModification;
  bool bCanEarlyOut = true;

  for (const ezString& dep : assetInfo.m_TransformDependencies)
  {
    if (!ezPathUtils::HasExtension(dep, "bank"))
      continue;

    {
      ezString sAssetFile = dep;
      if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
        continue;

      ezFileStats stat;
      if (ezOSFile::GetFileStats(sAssetFile, stat).Succeeded())
      {
        if (stat.m_LastModificationTime.Compare(cache.m_LastModification, ezTimestamp::CompareMode::Newer))
        {
          cache.m_LastModification = stat.m_LastModificationTime;
        }

        if (stat.m_LastModificationTime.Compare(lastTS, ezTimestamp::CompareMode::Newer))
        {
          bCanEarlyOut = false;
        }
      }
    }
  }

  if (bCanEarlyOut)
  {
    out_subAssets = cache.m_CachedSubAssets;
    return;
  }

  cache.m_CachedSubAssets.Clear();

  ezHashedString sAssetsDocumentTypeName;
  sAssetsDocumentTypeName.Assign("Sound Event");

  auto* pSystem = m_pFmod->GetSystem();
  ezHybridArray<FMOD::Studio::Bank*, 16> loadedBanks;


  // TODO: it is unclear whether the code below can produce deadlocks, because of locked soundbank files on disk
  // in theory, since m_TransformDependencies is alphabetically sorted, the access order should always be the same, and thus
  // no deadlock should be possible,

  for (const ezString& dep : assetInfo.m_TransformDependencies)
  {
    if (!ezPathUtils::HasExtension(dep, "bank"))
      continue;

    ezString sAssetFile = dep;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
      continue;

    FMOD::Studio::Bank* pBank = nullptr;
    auto res = pSystem->loadBankFile(sAssetFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pBank);
    if (res != FMOD_OK || pBank == nullptr)
      continue;

    loadedBanks.PushBack(pBank);

    ezStringBuilder sStringsBank = sAssetFile;
    sStringsBank.PathParentDirectory();
    sStringsBank.AppendPath("*.strings.bank");

    // honestly we have no idea what the strings bank name should be
    // and if there are multiple, which one is the correct one
    // so we just load everything that we can find
    ezFileSystemIterator fsIt;
    for (fsIt.StartSearch(sStringsBank, ezFileSystemIteratorFlags::ReportFiles); fsIt.IsValid(); fsIt.Next())
    {
      sStringsBank = fsIt.GetCurrentPath();
      sStringsBank.AppendPath(fsIt.GetStats().m_sName);

      FMOD::Studio::Bank* pStringsBank = nullptr;
      if (pSystem->loadBankFile(sStringsBank, FMOD_STUDIO_LOAD_BANK_NORMAL, &pStringsBank) == FMOD_OK && pStringsBank != nullptr)
      {
        loadedBanks.PushBack(pStringsBank);
      }
    }

    int iEvents = 0;
    EZ_FMOD_ASSERT(pBank->getEventCount(&iEvents));

    if (iEvents > 0)
    {
      ezDynamicArray<FMOD::Studio::EventDescription*> events;
      events.SetCountUninitialized(iEvents);

      pBank->getEventList(events.GetData(), iEvents, &iEvents);

      char szPath[256];
      int iLen;

      FMOD_GUID guid;

      ezStringBuilder sGuid, sGuidNoSpace, sEventName;

      for (ezUInt32 i = 0; i < events.GetCount(); ++i)
      {
        iLen = 0;
        EZ_FMOD_ASSERT(events[i]->getPath(szPath, 255, &iLen));
        szPath[iLen] = '\0';

        sEventName = szPath;

        if (sEventName.StartsWith_NoCase("snapshot:/"))
          continue;

        if (sEventName.StartsWith_NoCase("event:/"))
          sEventName.Shrink(7, 0);
        else
        {
          ezLog::Warning("Skipping unknown FMOD event type: '{0}", sEventName);
          continue;
        }

        events[i]->getID(&guid);

        ezUuid* ezGuid = reinterpret_cast<ezUuid*>(&guid);
        ezConversionUtils::ToString(*ezGuid, sGuid);
        sGuidNoSpace = sGuid;
        sGuidNoSpace.ReplaceAll(" ", "");

        auto& sub = cache.m_CachedSubAssets.ExpandAndGetRef();
        sub.m_Guid = *ezGuid;
        sub.m_sName = sEventName;
        sub.m_sSubAssetsDocumentTypeName = sAssetsDocumentTypeName;
      }
    }

    for (FMOD::Studio::Bank* pBank : loadedBanks)
    {
      EZ_FMOD_ASSERT(pBank->unload());
    }

    loadedBanks.Clear();
  }

  EZ_ASSERT_DEV(loadedBanks.IsEmpty(), "A soundbank wasn't unloaded.");

  out_subAssets = cache.m_CachedSubAssets;
}

ezString ezSoundBankAssetDocumentManager::GetSoundBankAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  // at the moment we don't reference the actual transformed asset file
  // instead we reference the source FMOD sound bank file
  // this makes development easier, as we don't need to wait for an asset transform before changes are available

  /// \todo For final release we should reference the transformed file, as it's the one that gets packaged etc.
  /// Maybe we should add another platform target for that ?

  // if (pAssetProfile == ezAssetCurator::GetSingleton()->GetDevelopmentAssetProfile())
  {
    for (const ezString& dep : pSubAsset->m_pAssetInfo->m_Info->m_TransformDependencies)
    {
      if (dep.EndsWith_NoCase(".bank") && !dep.EndsWith_NoCase(".strings.bank"))
      {
        ezStringBuilder result;
        result.Set("?", dep); // ? is an option to tell the system to skip the redirection prefix and use the path as is
        return result;
      }
    }
  }
  // else
  //{
  //  SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, pAssetProfile);
  //}

  return ezString();
}

ezString ezSoundBankAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return GetSoundBankAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);
  }
  else
  {
    ezStringBuilder result = GetSoundBankAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);

    ezStringBuilder sGuid;
    ezConversionUtils::ToString(pSubAsset->m_Data.m_Guid, sGuid);

    result.Append("|", sGuid);

    return result;
  }
}

void ezSoundBankAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSoundBankAssetDocument>())
      {
        new ezSoundBankAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void ezSoundBankAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezSoundBankAssetDocument(sPath);
}

void ezSoundBankAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

ezUInt64 ezSoundBankAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
