#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/OSFile.h>

#include <fmod_studio.hpp>
#define EZ_FMOD_ASSERT(res) EZ_VERIFY((res) == FMOD_OK, "Fmod failed with error code %u", res)


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundBankAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SoundBankFile", m_sSoundBank)->AddAttributes(new ezFileBrowserAttribute("Select SoundBank", "*.bank")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundBankAssetDocument::ezSoundBankAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezSoundBankAssetProperties>(szDocumentPath)
{
}

void ezSoundBankAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

  const ezSoundBankAssetProperties* pProp = GetProperties();

  pInfo->m_FileDependencies.Insert(pProp->m_sSoundBank);
}

FMOD::Studio::System* g_pSystem = nullptr;

ezStatus ezSoundBankAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  const ezSoundBankAssetProperties* pProp = GetProperties();

  EZ_LOG_BLOCK("SoundBank", pProp->m_sSoundBank.GetData());

  ezString sAssetFile = pProp->m_sSoundBank;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
    return ezStatus("SoundBank file '%s' does not exist", pProp->m_sSoundBank.GetData());

  if (g_pSystem == nullptr)
  {
    if (FMOD::Studio::System::create(&g_pSystem) != FMOD_OK)
      return ezStatus("Failed to initialize the Fmod system");

    //EZ_FMOD_ASSERT(g_pSystem->getLowLevelSystem(&m_pLowLevelSystem));
    //EZ_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

    void *extraDriverData = nullptr;
    EZ_FMOD_ASSERT(g_pSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, extraDriverData));
  }

  FMOD::Studio::Bank* pBank = nullptr;
  auto res = g_pSystem->loadBankFile(sAssetFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pBank);
  if (res != FMOD_OK)
    return ezStatus("SoundBank '%s' could not be loaded", pProp->m_sSoundBank.GetData());

  ezStringBuilder sStringsBank = sAssetFile;
  sStringsBank.RemoveFileExtension();
  sStringsBank.Append(".strings.bank");

  FMOD::Studio::Bank* pStringsBank = nullptr;
  g_pSystem->loadBankFile(sStringsBank, FMOD_STUDIO_LOAD_BANK_NORMAL, &pStringsBank);

  int iEvents = 0;
  EZ_FMOD_ASSERT(pBank->getEventCount(&iEvents));

  ezInt32 iStrings = 0;
  pStringsBank->getStringCount(&iStrings);
  ezLog::Info("SoundBank has %i strings", iStrings);

  for (ezInt32 i = 0; i < iStrings; ++i)
  {
    FMOD_GUID strGuid;
    char path[256];
    int len = 0;
    pStringsBank->getStringInfo(i, &strGuid, path, 255, &len);
    path[len] = '\0';

    ezLog::Debug("String %i: %s", i, path);
  }

  ezLog::Dev("SoundBank has %i events", iEvents);

  const ezString sOwnGuid = ezConversionUtils::ToString(GetGuid());

  ezStringBuilder sSubAssetsFile, sSubAssetLine;

  if (iEvents > 0)
  {
    ezDynamicArray<FMOD::Studio::EventDescription*> events;
    events.SetCount(iEvents);

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
      sGuid = ezConversionUtils::ToString(*ezGuid);
      sGuidNoSpace = sGuid;
      sGuidNoSpace.ReplaceAll(" ", "");

      ezLog::Info("Event: '%s' -> '%s'", sEventName.GetData(), sGuid.GetData());

      sSubAssetLine.Printf("%s;%s|%s;%s\n", sGuid.GetData(), sOwnGuid.GetData(), sGuidNoSpace.GetData(), sEventName.GetData());

      sSubAssetsFile.Append(sSubAssetLine.GetData());
    }
  }

  EZ_FMOD_ASSERT(pBank->unload());

  ezStringBuilder sSubAssetFilename = GetDocumentPath();
  sSubAssetFilename.Append(".ezSubAssets");

  {
    ezFileWriter file;
    if (file.Open(sSubAssetFilename).Failed())
      return ezStatus("Failed to write sub-assets file '%s'", sSubAssetFilename.GetData());
    file.WriteBytes(sSubAssetsFile.GetData(), sSubAssetsFile.GetElementCount());
  }

  return ezStatus(EZ_SUCCESS);
}
