#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundBankAsset/SoundBankPreview.h>
#include <FmodPlugin/FmodIncludes.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

#include <fmod_errors.h>

ezSoundBankPreview::ezSoundBankPreview()
{
  FMOD_RESULT res = FMOD::Studio::System::create(&m_pSystem);
  if (res != FMOD_OK)
  {
    ezLog::Error("Failed to create the FMOD system for the sound preview: {}", FMOD_ErrorString(res));
    m_pSystem = nullptr;
    return;
  }

  res = m_pSystem->initialize(32, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
  if (res != FMOD_OK)
  {
    ezLog::Error("Failed to initialize the FMOD system for the sound preview: {}", FMOD_ErrorString(res));
    m_pSystem->release();
    m_pSystem = nullptr;
  }
}

ezSoundBankPreview::~ezSoundBankPreview()
{
  if (m_pSystem == nullptr)
    return;

  UnloadBanks();

  // also unloads everything that is still loaded
  m_pSystem->release();
  m_pSystem = nullptr;
}

ezStatus ezSoundBankPreview::LoadBank(ezStringView sAbsolutePath)
{
  UnloadBanks();

  if (m_pSystem == nullptr)
    return ezStatus("FMOD could not be initialized.");

  FMOD::Studio::Bank* pBank = nullptr;

  const ezString sBankPath = sAbsolutePath;
  FMOD_RESULT res = m_pSystem->loadBankFile(sBankPath, FMOD_STUDIO_LOAD_BANK_NORMAL, &pBank);
  if (res != FMOD_OK || pBank == nullptr)
    return ezStatus(ezFmt("Failed to load sound bank '{}': {}", sAbsolutePath, FMOD_ErrorString(res)));

  // The name of the master bank isn't known, but each master bank comes with a strings bank of the same name.
  // Failures are fine here, for example when the requested bank is the master bank, it's already loaded.
  {
    ezStringBuilder sSearch = sAbsolutePath;
    sSearch.PathParentDirectory();
    sSearch.AppendPath("*.strings.bank");

    ezStringBuilder sFile;

    ezFileSystemIterator fsIt;
    for (fsIt.StartSearch(sSearch, ezFileSystemIteratorFlags::ReportFiles); fsIt.IsValid(); fsIt.Next())
    {
      FMOD::Studio::Bank* pOtherBank = nullptr;

      sFile = fsIt.GetCurrentPath();
      sFile.AppendPath(fsIt.GetStats().m_sName);
      m_pSystem->loadBankFile(sFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pOtherBank);

      sFile.ReplaceLast(".strings.bank", ".bank");
      m_pSystem->loadBankFile(sFile, FMOD_STUDIO_LOAD_BANK_NORMAL, &pOtherBank);
    }
  }

  int iEvents = 0;
  pBank->getEventCount(&iEvents);

  if (iEvents > 0)
  {
    ezDynamicArray<FMOD::Studio::EventDescription*> descriptions;
    descriptions.SetCountUninitialized(iEvents);
    pBank->getEventList(descriptions.GetData(), iEvents, &iEvents);

    for (int i = 0; i < iEvents; ++i)
    {
      char szPath[256] = {};
      int iLen = 0;

      ezStringBuilder sName;
      if (descriptions[i]->getPath(szPath, EZ_ARRAY_SIZE(szPath), &iLen) == FMOD_OK)
        sName = szPath;

      // same filter as for the sub-assets
      if (sName.StartsWith_NoCase("snapshot:/"))
        continue;

      if (sName.StartsWith_NoCase("event:/"))
        sName.Shrink(7, 0);

      FMOD_GUID guid;
      descriptions[i]->getID(&guid);

      int iLengthMs = 0;
      descriptions[i]->getLength(&iLengthMs);

      Event& ev = m_Events.ExpandAndGetRef();
      ev.m_Guid = *reinterpret_cast<ezUuid*>(&guid);
      ev.m_Length = ezTime::MakeFromMilliseconds(iLengthMs);
      ev.m_pDescription = descriptions[i];
      descriptions[i]->is3D(&ev.m_b3D);
      descriptions[i]->isOneshot(&ev.m_bOneShot);

      if (sName.IsEmpty())
      {
        // no strings bank, the GUID is all we have
        ezConversionUtils::ToString(ev.m_Guid, sName);
      }

      ev.m_sName = sName;
    }
  }

  m_Events.Sort([](const Event& a, const Event& b)
    { return a.m_sName.Compare_NoCase(b.m_sName) < 0; });

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSoundBankPreview::Play(ezUInt32 uiEventIndex)
{
  Stop();

  if (m_pSystem == nullptr || uiEventIndex >= m_Events.GetCount())
    return ezStatus("Invalid sound event.");

  FMOD_RESULT res = m_Events[uiEventIndex].m_pDescription->createInstance(&m_pInstance);
  if (res != FMOD_OK)
  {
    m_pInstance = nullptr;
    return ezStatus(ezFmt("Failed to create sound event: {}", FMOD_ErrorString(res)));
  }

  res = m_pInstance->start();
  if (res != FMOD_OK)
  {
    Stop();
    return ezStatus(ezFmt("Failed to start sound event: {}", FMOD_ErrorString(res)));
  }

  Update();

  return ezStatus(EZ_SUCCESS);
}

void ezSoundBankPreview::Stop()
{
  if (m_pInstance == nullptr)
    return;

  m_pInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
  m_pInstance->release();
  m_pInstance = nullptr;

  // nothing else calls update() once playback is over
  Update();
}

void ezSoundBankPreview::Update()
{
  if (m_pSystem != nullptr)
  {
    m_pSystem->update();
  }
}

bool ezSoundBankPreview::IsPlaying() const
{
  if (m_pInstance == nullptr)
    return false;

  FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
  if (m_pInstance->getPlaybackState(&state) != FMOD_OK)
    return false;

  return state != FMOD_STUDIO_PLAYBACK_STOPPED;
}

ezTime ezSoundBankPreview::GetPosition() const
{
  int iPositionMs = 0;

  if (m_pInstance != nullptr)
  {
    m_pInstance->getTimelinePosition(&iPositionMs);
  }

  return ezTime::MakeFromMilliseconds(iPositionMs);
}

void ezSoundBankPreview::UnloadBanks()
{
  Stop();

  m_Events.Clear();

  if (m_pSystem != nullptr)
  {
    m_pSystem->unloadAll();
    m_pSystem->update();
  }
}
