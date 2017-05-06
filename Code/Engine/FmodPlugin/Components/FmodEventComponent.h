#pragma once

#include <FmodPlugin/Components/FmodComponent.h>
#include <Core/Messages/ScriptFunctionMessage.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/Messages/EventMessage.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class ezFmodEventComponentManager : public ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating>
{
public:
  ezFmodEventComponentManager(ezWorld* pWorld);
  
  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  void ResourceEventHandler(const ezResourceEvent& e);
};

typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;

struct ezResourceEvent;

//////////////////////////////////////////////////////////////////////////

struct EZ_FMODPLUGIN_DLL ezFmodEventComponent_RestartSoundMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodEventComponent_RestartSoundMsg, ezScriptFunctionMessage);

  /// If true, a separate one-shot instance is started, which will play till completion. 
  /// If false, the sound is played on the component itself, which allows to move it, restart, stop it, etc.
  bool m_bOneShotInstance = true;
};

struct EZ_FMODPLUGIN_DLL ezFmodEventComponent_StopSoundMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodEventComponent_StopSoundMsg, ezScriptFunctionMessage);

  /// If true, the sound is interrupted immediately. Otherwise it is allows to fade out.
  bool m_bImmediate = false;
};

/// \brief Sent when a ezFmodEventComponent finishes playing a sound. Not sent for one-shot sound events.
struct EZ_FMODPLUGIN_DLL ezFmodSoundFinishedEventMessage : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodSoundFinishedEventMessage, ezEventMessage);

};

/// \brief Triggers an fmod sound cue on the sound event.
struct EZ_FMODPLUGIN_DLL ezFmodEventComponent_SoundCueMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezFmodEventComponent_SoundCueMsg, ezScriptFunctionMessage);
};

//////////////////////////////////////////////////////////////////////////

class EZ_FMODPLUGIN_DLL ezFmodEventComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodEventComponent, ezFmodComponent, ezFmodEventComponentManager);
  virtual void ezFmodComponentIsAbstract() override {}
  friend class ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating>;

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

protected:
  bool m_bPaused;
  float m_fPitch;
  float m_fVolume;
  ezInt32 m_iTimelinePosition = -1; // used to restore a sound after reloading the resource
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

  /// \brief Starts playing a new sound.
  ///
  /// One-shot sounds play in parallel.
  /// For not one-shot sounds, if it is already playing, it is interrupted and started from the beginning.
  void RestartSound(ezFmodEventComponent_RestartSoundMsg& msg);

  /// Stops the current sound from playing. Typically allows the sound to fade out briefly, unless specified otherwise.
  void StopSound(ezFmodEventComponent_StopSoundMsg& msg);

  /// \brief Triggers an fmod sound cue. Whatever that is useful for.
  void SoundCue(ezFmodEventComponent_SoundCueMsg& msg);

  /// \brief Tries to find the fmod event parameter by name. Returns the parameter index or -1, if no such parameter exists.
  ezInt32 FindParameter(const char* szName) const;

  /// \brief Sets an fmod event parameter value. See FindParameter() for the index.
  void SetParameter(ezInt32 iParamIndex, float fValue);

  /// \brief Gets an fmod event parameter value. See FindParameter() for the index. Returns 0, if the index is invalid.
  float GetParameter(ezInt32 iParamIndex) const;

protected:

  void Update();
  void SetParameters3d(FMOD::Studio::EventInstance* pEventInstance);

  /// Called when the event resource has been unloaded (for a reload)
  void InvalidateResource(bool bTryToRestore);

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;

  ezEventMessageSender<ezFmodSoundFinishedEventMessage> m_SoundFinishedEventSender;
};


//////////////////////////////////////////////////////////////////////////

class EZ_FMODPLUGIN_DLL ezVisualScriptNode_SetFmodEventParameter : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetFmodEventParameter, ezVisualScriptNode);
public:
  ezVisualScriptNode_SetFmodEventParameter();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  const char* GetParameterName() const { return m_sParameterName.GetData(); }
  void SetParameterName(const char* sz) { m_sParameterName.Assign(sz); }

private:
  ezComponentHandle m_hComponent;
  double m_fValue;
  ezInt32 m_iParameterIndex = -1;
  ezHashedString m_sParameterName;
};