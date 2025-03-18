#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

struct ezMiniAudioSoundInstance;
using ezMiniAudioSoundResourceHandle = ezTypedResourceHandle<class ezMiniAudioSoundResource>;

class ezMiniAudioSoundComponentManager : public ezComponentManager<class ezMiniAudioSoundComponent, ezBlockStorageType::FreeList>
{
public:
  ezMiniAudioSoundComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezMiniAudioSoundComponent;

  void UpdateEvents(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_MINIAUDIOPLUGIN_DLL ezMiniAudioSoundComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMiniAudioSoundComponent, ezComponent, ezMiniAudioSoundComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezMiniAudioComponent

private:
  friend class ezComponentManagerSimple<class ezMiniAudioSoundComponent, ezComponentUpdateType::WhenSimulating>;


  //////////////////////////////////////////////////////////////////////////
  // ezMiniAudioSoundComponent

public:
  ezMiniAudioSoundComponent();
  ~ezMiniAudioSoundComponent();

  void SetPaused(bool b);                                // [ property ]
  bool GetPaused() const { return m_bPaused; }           // [ property ]

  void SetPitch(float f);                                // [ property ]
  float GetPitch() const { return m_fPitch; }            // [ property ]

  void SetVolume(float f);                               // [ property ]
  float GetVolume() const { return m_fComponentVolume; } // [ property ]

  /// \brief If set, the global game speed does not affect the pitch of this event.
  ///
  /// This is important for global sounds, such as music or UI effects, so that they always play at their regular speed,
  /// even when the game is in slow motion.
  void SetNoGlobalPitch(bool bEnable);                     // [ property ]
  bool GetNoGlobalPitch() const;                           // [ property ]

  ezEnum<ezOnComponentFinishedAction2> m_OnFinishedAction; // [ property ]

  /// \brief Makes the sound play.
  ///
  /// If it was not yet playing, it starts playing a new sound.
  /// If it was already playing, but paused, playback is resumed.
  /// If it was already playing, there is no change.
  void Play(); // [ scriptable ]

  /// \brief If a sound is playing, it pauses at the current play position.
  ///
  /// Call Play() to resume playing.
  void Pause(); // [ scriptable ]

  /// \brief Interrupts the sound playback abruptly.
  void Stop(); // [ scriptable ]

  /// \brief Stops the sound, by fading it out over a short period.
  void FadeOut(ezTime fadeDuration); // [ scriptable ]

  /// \brief Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position and direction are copied to the new sound instance.
  /// The new sound then plays to the end and cannot be controlled through this component any further.
  /// No sound should be playing on this component, when using this function.
  void StartOneShot();                                    // [ scriptable ]

protected:
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg); // [ msg handler ]

  void Update(bool bForce);

  friend class ezMiniAudioSingleton;
  void SoundFinished();

  ezMiniAudioSoundResourceHandle m_hSound;
  float m_fComponentVolume = 1.0f;
  float m_fPitch = 1.0f;
  float m_fResourceVolume = 1.0f;
  float m_fResourcePitch = 1.0f;
  bool m_bPaused = false;
  ezUInt8 m_uiVolumeChanged : 1;
  ezUInt8 m_uiPitchChanged : 1;

  ezMiniAudioSoundInstance* m_pInstance = nullptr;

};
