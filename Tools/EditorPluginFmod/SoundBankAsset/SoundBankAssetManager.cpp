#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <FmodPlugin/FmodIncludes.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundBankAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class ezSimpleFmod
{
public:
  ezSimpleFmod() {}
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

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Sound Bank Asset";
  m_AssetDesc.m_sFileExtension = "ezSoundBankAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Sound_Bank.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezSoundBankAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Sound Bank", QPixmap(":/AssetIcons/Sound_Bank.png"));

  m_Fmod = EZ_DEFAULT_NEW(ezSimpleFmod);
}

ezSoundBankAssetDocumentManager::~ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  m_Fmod->Shutdown();
  m_Fmod.Reset();
}

void ezSoundBankAssetDocumentManager::FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo,
                                                          ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const
{
  ezHashedString sAssetTypeName;
  sAssetTypeName.Assign("Sound Event");

  auto* pSystem = m_Fmod->GetSystem();

  ezHybridArray<FMOD::Studio::Bank*, 16> loadedBanks;

  for (const ezString& dep : assetInfo.m_AssetTransformDependencies)
  {
    if (!ezPathUtils::HasExtension(dep, "bank"))
      continue;

    {
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
      if (fsIt.StartSearch(sStringsBank, false, false).Succeeded())
      {
        do
        {
          sStringsBank = fsIt.GetCurrentPath();
          sStringsBank.AppendPath(fsIt.GetStats().m_sFileName);

          FMOD::Studio::Bank* pStringsBank = nullptr;
          if (pSystem->loadBankFile(sStringsBank, FMOD_STUDIO_LOAD_BANK_NORMAL, &pStringsBank) == FMOD_OK && pStringsBank != nullptr)
          {
            loadedBanks.PushBack(pStringsBank);
          }
        } while (fsIt.Next().Succeeded());
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
          auto ret = events[i]->getPath(szPath, 255, &iLen);
          szPath[iLen] = '\0';

          sEventName = szPath;

          if (sEventName.StartsWith_NoCase("snapshot:/"))
            continue;

          if (sEventName.StartsWith_NoCase("event:/"))
            sEventName.Shrink(7, 0);
          else
          {
            ezLog::Warning("Skipping unknown fmod event type: '{0}", sEventName);
            continue;
          }

          events[i]->getID(&guid);

          ezUuid* ezGuid = reinterpret_cast<ezUuid*>(&guid);
          ezConversionUtils::ToString(*ezGuid, sGuid);
          sGuidNoSpace = sGuid;
          sGuidNoSpace.ReplaceAll(" ", "");

          auto& sub = out_SubAssets.ExpandAndGetRef();
          sub.m_Guid = *ezGuid;
          sub.m_sName = sEventName;
          sub.m_sAssetTypeName = sAssetTypeName;
        }
      }
    }
  }

  for (FMOD::Studio::Bank* pBank : loadedBanks)
  {
    EZ_FMOD_ASSERT(pBank->unload());
  }
}

ezString ezSoundBankAssetDocumentManager::GetSoundBankAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory,
                                                                      const char* szPlatform) const
{
  // at the moment we don't reference the actual transformed asset file
  // instead we reference the source fmod sound bank file
  // this makes development easier, as we don't need to wait for an asset transform before changes are available

  /// \todo For final release we should reference the transformed file, as it's the one that gets packaged etc.
  /// Maybe we should add another platform target for that ?

  if (ezStringUtils::IsEqual(szPlatform, ezAssetCurator::GetSingleton()->GetDevelopmentPlatform()))
  {
    for (const ezString& dep : pSubAsset->m_pAssetInfo->m_Info->m_AssetTransformDependencies)
    {
      if (dep.EndsWith_NoCase(".bank") && !dep.EndsWith_NoCase(".strings.bank"))
      {
        ezStringBuilder result;
        result.Set("?", dep); // ? is an option to tell the system to skip the redirection prefix and use the path as is
        return result;
      }
    }
  }
  else
  {
    SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, szPlatform);
  }

  return ezString();
}

ezString ezSoundBankAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory,
                                                             const char* szPlatform) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return GetSoundBankAssetTableEntry(pSubAsset, szDataDirectory, szPlatform);
  }
  else
  {
    ezStringBuilder result = GetSoundBankAssetTableEntry(pSubAsset, szDataDirectory, szPlatform);

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
        ezSoundBankAssetDocumentWindow* pDocWnd = new ezSoundBankAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezSoundBankAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                 bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSoundBankAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSoundBankAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
