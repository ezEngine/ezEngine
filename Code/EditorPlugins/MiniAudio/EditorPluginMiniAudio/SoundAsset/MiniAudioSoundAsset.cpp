#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSoundAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSoundAssetProperties, 1, ezRTTIDefaultAllocator<ezMiniAudioSoundAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Files", m_SoundFiles)->AddAttributes(new ezFileBrowserAttribute("Select Sound", "*.wav;*.mp3")),
    EZ_MEMBER_PROPERTY("Loop", m_bLoop),
    EZ_MEMBER_PROPERTY("MinRandomVolume", m_fMinVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_MEMBER_PROPERTY("MaxRandomVolume", m_fMaxVolume)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_MEMBER_PROPERTY("MinRandomPitch", m_fMinPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_MEMBER_PROPERTY("MaxRandomPitch", m_fMaxPitch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_MEMBER_PROPERTY("IsPositional", m_bSpatialize)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("DopplerFactor", m_fDopplerFactor)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("SoundSize", m_fMinDistance)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.01f, 100.0f)),
    // EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(1.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("Rolloff", m_fRolloff)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.001f, 1000.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMiniAudioSoundAssetDocument::ezMiniAudioSoundAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezMiniAudioSoundAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezMiniAudioSoundAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezMiniAudioSoundAssetProperties* pProp = GetProperties();

  for (const auto& str : pProp->m_SoundFiles)
  {
    pInfo->m_TransformDependencies.Insert(str);
  }
}

ezTransformStatus ezMiniAudioSoundAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezMiniAudioSoundAssetProperties* pProp = GetProperties();

  if (pProp->m_SoundFiles.IsEmpty())
    return ezStatus("No sound files have been specified.");

  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << pProp->m_bLoop;
  stream << pProp->m_fMinVolume;
  stream << pProp->m_fMaxVolume;
  stream << pProp->m_fMinPitch;
  stream << pProp->m_fMaxPitch;
  stream << pProp->m_bSpatialize;
  stream << pProp->m_fMinDistance;
  stream << pProp->m_fMaxDistance;
  stream << pProp->m_fRolloff;
  stream << pProp->m_fDopplerFactor;
  stream << pProp->m_SoundFiles.GetCount();

  for (const auto& sFile : pProp->m_SoundFiles)
  {
    ezStringBuilder sAssetFile = sFile;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
      return ezStatus(ezFmt("Failed to make sound file path absolute: '{0}'", sFile));

    ezFileReader SoundFile;
    if (SoundFile.Open(sAssetFile).Failed())
      return ezStatus(ezFmt("Could not open sound-file for reading: '{0}'", sAssetFile));

    // we copy the entire sound into our transformed asset

    ezDefaultMemoryStreamStorage storage;

    // copy the file from disk into memory
    {
      ezMemoryStreamWriter writer(&storage);

      ezUInt8 Temp[4 * 1024];

      while (true)
      {
        ezUInt64 uiRead = SoundFile.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));

        if (uiRead == 0)
          break;

        writer.WriteBytes(Temp, uiRead).IgnoreResult();
      }
    }

    // now store the entire file in our asset output
    stream << storage.GetStorageSize32();
    EZ_SUCCEED_OR_RETURN(storage.CopyToStream(stream));
  }

  return ezStatus(EZ_SUCCESS);
}

void ezMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezMiniAudioSoundAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool bIsPositional = e.m_pObject->GetTypeAccessor().GetValue("IsPositional").ConvertTo<bool>();

    props["DopplerFactor"].m_Visibility = bIsPositional ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SoundSize"].m_Visibility = bIsPositional ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["Rolloff"].m_Visibility = bIsPositional ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
}
