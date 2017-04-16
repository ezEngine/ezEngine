#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundBankAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundBankAssetDocumentManager::ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  //ezAssetFileExtensionWhitelist::AddAssetFileExtension("Collision Mesh", "ezFmodMesh");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Sound Bank Asset";
  m_AssetDesc.m_sFileExtension = "ezSoundBankAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Sound_Bank.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezSoundBankAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezSoundBankAssetDocumentManager::~ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));
}

extern FMOD::Studio::System* g_pSystem;

void ezSoundBankAssetDocumentManager::FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezHybridArray<ezSubAssetData, 4>& out_SubAssets) const
{
  ezHashedString sAssetTypeName;
  sAssetTypeName.Assign("Sound Event");

  for (const ezString& dep : assetInfo.m_FileDependencies)
  {
    if (!ezPathUtils::HasExtension(dep, "bank"))
      continue;

    {
      ezString sAssetFile = dep;
      if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
        continue;
        //return ezStatus(ezFmt("SoundBank file '{0}' does not exist", sAssetFile));

      if (g_pSystem == nullptr)
      {
        if (FMOD::Studio::System::create(&g_pSystem) != FMOD_OK)
          continue;
          //return ezStatus("Failed to initialize the Fmod system");

        //EZ_FMOD_ASSERT(g_pSystem->getLowLevelSystem(&m_pLowLevelSystem));
        //EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

        void *extraDriverData = nullptr;
        EZ_FMOD_ASSERT(g_pSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData));
      }

      FMOD::Studio::Bank* pBank = nullptr;
      auto res = g_pSystem->loadBankFile(sAssetFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pBank);
      if (res != FMOD_OK)
        continue;
        //return ezStatus(ezFmt("SoundBank '{0}' could not be loaded", pProp->m_sSoundBank.GetData()));

      ezStringBuilder sStringsBank = sAssetFile;
      sStringsBank.RemoveFileExtension();
      sStringsBank.Append(".strings.bank");

      FMOD::Studio::Bank* pStringsBank = nullptr;
      g_pSystem->loadBankFile(sStringsBank, FMOD_STUDIO_LOAD_BANK_NORMAL, &pStringsBank);

      int iEvents = 0;
      EZ_FMOD_ASSERT(pBank->getEventCount(&iEvents));

      //ezInt32 iStrings = 0;
      //pStringsBank->getStringCount(&iStrings);
      //ezLog::Info("SoundBank has {0} strings", iStrings);

      //for (ezInt32 i = 0; i < iStrings; ++i)
      //{
      //  FMOD_GUID strGuid;
      //  char path[256];
      //  int len = 0;
      //  pStringsBank->getStringInfo(i, &strGuid, path, 255, &len);
      //  path[len] = '\0';

      //  //ezLog::Debug("String {0}: {1}", i, path);
      //}

      //ezLog::Dev("SoundBank has {0} events", iEvents);

      //ezStringBuilder sOwnGuid;
      //ezConversionUtils::ToString(GetGuid(), sOwnGuid);

      //ezStringBuilder sSubAssetsFile, sSubAssetLine;

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
          if (sEventName.StartsWith_NoCase("event:/"))
            sEventName.Shrink(7, 0);

          events[i]->getID(&guid);

          ezUuid* ezGuid = reinterpret_cast<ezUuid*>(&guid);
          ezConversionUtils::ToString(*ezGuid, sGuid);
          sGuidNoSpace = sGuid;
          sGuidNoSpace.ReplaceAll(" ", "");

          //ezLog::Info("Event: '{0}' -> '{1}'", sEventName.GetData(), sGuid.GetData());

          //sSubAssetLine.Format("{0};{1}|{2};{3}\n", sGuid.GetData(), sOwnGuid.GetData(), sGuidNoSpace.GetData(), sEventName.GetData());

          //sSubAssetsFile.Append(sSubAssetLine.GetData());

          auto& sub = out_SubAssets.ExpandAndGetRef();
          sub.m_Guid = *ezGuid;
          sub.m_sName = sEventName;
          sub.m_sAssetTypeName = sAssetTypeName;
        }
      }

      EZ_FMOD_ASSERT(pBank->unload());

      //ezStringBuilder sSubAssetFilename = GetDocumentPath();
      //sSubAssetFilename.Append(".ezSubAssets");

      //{
      //  ezFileWriter file;
      //  if (file.Open(sSubAssetFilename).Failed())
      //    return ezStatus(ezFmt("Failed to write sub-assets file '{0}'", sSubAssetFilename.GetData()));

      //  file.WriteBytes(sSubAssetsFile.GetData(), sSubAssetsFile.GetElementCount());
      //}

    }
  }
}


ezString ezSoundBankAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const char* szPlatform) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, szPlatform);
  }
  else
  {
    ezStringBuilder sGuid;
    ezConversionUtils::ToString(pSubAsset->m_Data.m_Guid, sGuid);

    ezStringBuilder result = SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, szPlatform);
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

ezStatus ezSoundBankAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSoundBankAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSoundBankAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSoundBankAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



