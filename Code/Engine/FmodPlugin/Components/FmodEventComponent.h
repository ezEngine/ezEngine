#pragma once

#include <FmodPlugin/Components/FmodComponent.h>
#include <Core/ResourceManager/Resource.h>

typedef ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating> ezFmodEventComponentManager;
typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;

//////////////////////////////////////////////////////////////////////////

struct ezFmodEventComponent_RestartSoundMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodEventComponent_RestartSoundMsg, ezScriptFunctionMessage);

  bool m_bOneShotInstance = true;
};

struct ezFmodEventComponent_StopSoundMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodEventComponent_StopSoundMsg, ezScriptFunctionMessage);

  bool m_bImmediate = false;
};


//////////////////////////////////////////////////////////////////////////

class EZ_FMODPLUGIN_DLL ezFmodEventComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodEventComponent, ezFmodComponent, ezFmodEventComponentManager);
  virtual void ezFmodComponentIsAbstract() override {}

public:
  ezFmodEventComponent();
  ~ezFmodEventComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  void SetPaused(bool b);
  bool GetPaused() const { return m_bPaused; }

  void SetPitch(float f);
  float GetPitch() const { return m_fPitch; }

  void SetVolume(float f);
  float GetVolume() const { return m_fVolume; }

  void SetSoundEventFile(const char* szFile);
  const char* GetSoundEventFile() const;

  void SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent);
  const ezFmodSoundEventResourceHandle& GetSoundEvent() const { return m_hSoundEvent; }

  ezOnComponentFinishedAction::Enum m_OnFinishedAction;

  /// \todo Event Parameters (expose, modify)

protected:
  bool m_bPaused;
  float m_fPitch;
  float m_fVolume;
  ezFmodSoundEventResourceHandle m_hSoundEvent;


  // ************************************* FUNCTIONS *****************************

public:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  /// \brief Will start the sound, if it was not playing. Will restart the sound, if it was already playing.
  /// If the sound was paused so far, this will change the paused state to playing.
  void Restart();

  /// \brief Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position, direction and velocity are copied to the new sound instance.
  /// The new sound event then plays to the end and cannot be controlled through this component any further.
  /// If the referenced fmod sound event is not a "one shot" event, this function is ignored.
  /// The event that is controlled through this component is unaffected by this.
  void StartOneShot();

  void RestartSound(ezFmodEventComponent_RestartSoundMsg& msg);
  void StopSound(ezFmodEventComponent_StopSoundMsg& msg);

protected:

  void Update();
  void SetParameters3d(FMOD::Studio::EventInstance* pEventInstance);

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;
};


