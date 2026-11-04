#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

namespace FMOD::Studio
{
  class System;
  class EventDescription;
  class EventInstance;
} // namespace FMOD::Studio

/// Lists the events in a sound bank and plays them in the editor process, for the preview in the sound bank window.
///
/// Uses its own FMOD system, because ezSoundBankAssetDocumentManager loads and unloads the same banks to determine the sub-assets,
/// and FMOD refuses to load a bank twice into the same system.
/// The FMOD system is created with this object, so an audio device is held for as long as it exists.
class ezSoundBankPreview
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSoundBankPreview);

public:
  struct Event
  {
    ezUuid m_Guid;
    ezString m_sName;
    ezTime m_Length; ///< Zero if the event has no timeline.
    bool m_b3D = false;
    bool m_bOneShot = false;
    FMOD::Studio::EventDescription* m_pDescription = nullptr;
  };

  ezSoundBankPreview();
  ~ezSoundBankPreview();

  /// Replaces the currently loaded bank. Any playing event is stopped.
  ///
  /// The master and strings banks in the same folder are loaded as well, events can't be played or be identified by name without them.
  /// On failure, no events are available and the reason is returned.
  ezStatus LoadBank(ezStringView sAbsolutePath);

  const ezDynamicArray<Event>& GetEvents() const { return m_Events; }

  /// Stops any previous event and starts the given one. Parameters of the event keep their default values.
  ezStatus Play(ezUInt32 uiEventIndex);

  void Stop();

  /// Has to be called regularly during playback, FMOD doesn't advance without it.
  void Update();

  /// Returns false once the event has ended by itself.
  bool IsPlaying() const;

  ezTime GetPosition() const;

private:
  void UnloadBanks();

  FMOD::Studio::System* m_pSystem = nullptr;
  FMOD::Studio::EventInstance* m_pInstance = nullptr;
  ezDynamicArray<Event> m_Events;
};
