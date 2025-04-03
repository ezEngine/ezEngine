#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/Components/FmodComponent.h>

struct ezFmodParameterId
{
public:
  EZ_ALWAYS_INLINE void Invalidate() { m_uiValue = -1; }
  EZ_ALWAYS_INLINE bool IsInvalidated() const { return m_uiValue == -1; }

private:
  ezUInt64 m_uiValue = -1;
};

class ezPhysicsWorldModuleInterface;
struct ezMsgSetFloatParameter;

class ezFmodEventComponentManager : public ezComponentManager<class ezFmodEventComponent, ezBlockStorageType::FreeList>
{
public:
  ezFmodEventComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezFmodEventComponent;

  struct OcclusionState
  {
    ezFmodEventComponent* m_pComponent = nullptr;
    ezFmodParameterId m_OcclusionParamId;
    ezUInt32 m_uiRaycastHits = 0;
    ezUInt8 m_uiNextRayIndex = 0;
    ezUInt8 m_uiNumUsedRays = 0;
    float m_fRadius = 0.0f;
    float m_fLastOcclusionValue = -1.0f;

    float GetOcclusionValue(float fThreshold) const { return ezMath::Clamp((m_fLastOcclusionValue - fThreshold) / ezMath::Max(1.0f - fThreshold, 0.0001f), 0.0f, 1.0f); }
  };

  ezUInt32 m_uiFirstComponentIndex = 0;
  ezDynamicArray<OcclusionState> m_OcclusionStates;

  ezUInt32 AddOcclusionState(ezFmodEventComponent* pComponent, ezFmodParameterId occlusionParamId, float fRadius);
  void RemoveOcclusionState(ezUInt32 uiIndex);
  const OcclusionState& GetOcclusionState(ezUInt32 uiIndex) const { return m_OcclusionStates[uiIndex]; }

  void ShootOcclusionRays(
    OcclusionState& state, ezVec3 listenerPos, ezUInt32 uiNumRays, const ezPhysicsWorldModuleInterface* pPhysicsWorldModule, ezTime deltaTime);
  void UpdateOcclusion(const ezWorldModule::UpdateContext& context);
  void UpdateEvents(const ezWorldModule::UpdateContext& context);

  void ResourceEventHandler(const ezResourceEvent& e);
};

using ezFmodSoundEventResourceHandle = ezTypedResourceHandle<class ezFmodSoundEventResource>;

struct ezResourceEvent;

//////////////////////////////////////////////////////////////////////////

/// \brief Sent when a ezFmodEventComponent finishes playing a sound. Not sent for one-shot sound events.
struct EZ_FMODPLUGIN_DLL ezMsgFmodSoundFinished : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgFmodSoundFinished, ezEventMessage);
};


//////////////////////////////////////////////////////////////////////////

/// \brief Represents a sound (called an 'event') in the FMOD sound system.
///
/// Provides functions to start, pause, stop a sound, set parameters, change volume, pitch etc.
class EZ_FMODPLUGIN_DLL ezFmodEventComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodEventComponent, ezFmodComponent, ezFmodEventComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezFmodComponent

private:
  virtual void ezFmodComponentIsAbstract() override {}
  friend class ezComponentManagerSimple<class ezFmodEventComponent, ezComponentUpdateType::WhenSimulating>;


  //////////////////////////////////////////////////////////////////////////
  // ezFmodEventComponent

public:
  ezFmodEventComponent();
  ~ezFmodEventComponent();

  void SetPaused(bool b);                                                               // [ property ]
  bool GetPaused() const { return m_bPaused; }                                          // [ property ]

  void SetUseOcclusion(bool b);                                                         // [ property ]
  bool GetUseOcclusion() const { return m_bUseOcclusion; }                              // [ property ]

  void SetOcclusionCollisionLayer(ezUInt8 uiCollisionLayer);                            // [ property ]
  ezUInt8 GetOcclusionCollisionLayer() const { return m_uiOcclusionCollisionLayer; }    // [ property ]

  void SetOcclusionThreshold(float fThreshold);                                         // [ property ]
  float GetOcclusionThreshold() const;                                                  // [ property ]

  void SetPitch(float f);                                                               // [ property ]
  float GetPitch() const { return m_fPitch; }                                           // [ property ]

  void SetVolume(float f);                                                              // [ property ]
  float GetVolume() const { return m_fVolume; }                                         // [ property ]

  void SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent);                // [ property ]
  const ezFmodSoundEventResourceHandle& GetSoundEvent() const { return m_hSoundEvent; } // [ property ]

  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction;                               // [ property ]

  void SetShowDebugInfo(bool bShow);                                                    // [ property ]
  bool GetShowDebugInfo() const;                                                        // [ property ]

  /// \brief If set, the global game speed does not affect the pitch of this event.
  ///
  /// This is important for global sounds, such as music or UI effects, so that they always play at their regular speed,
  /// even when the game is in slow motion.
  void SetNoGlobalPitch(bool bEnable); // [ property ]
  bool GetNoGlobalPitch() const;       // [ property ]

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
  void FadeOut(); // [ scriptable ]

  /// \brief Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position, direction and velocity are copied to the new sound instance.
  /// The new sound event then plays to the end and cannot be controlled through this component any further.
  /// If the referenced FMOD sound event is not a "one shot" event, this function is ignored.
  /// The event that is controlled through this component is unaffected by this.
  void StartOneShot(); // [ scriptable ]

  /// \brief Triggers an FMOD sound cue. Whatever that is useful for.
  void SoundCue(); // [ scriptable ]

  /// \brief Tries to find the FMOD event parameter by name. Returns the parameter id or -1, if no such parameter exists.
  ezFmodParameterId FindParameter(const char* szName) const;

  /// \brief Sets an FMOD event parameter value. See FindParameter() for the index.
  void SetParameter(ezFmodParameterId paramId, float fValue);

  /// \brief Gets an FMOD event parameter value. See FindParameter() for the index. Returns 0, if the index is invalid.
  float GetParameter(ezFmodParameterId paramId) const;

  /// \brief Sets an event parameter via name lookup, so this is less efficient than SetParameter()
  void SetEventParameter(const char* szParamName, float fValue); // [ scriptable ]

  /// \brief Allows one to set event parameters through the generic ezMsgSetFloatParameter message.
  ///
  /// Requires event parameter lookup via a name, so this is less efficient than SetParameter().
  void OnMsgSetFloatParameter(ezMsgSetFloatParameter& ref_msg); // [ msg handler ]

protected:
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);       // [ msg handler ]

  void Update();
  void UpdateParameters(FMOD::Studio::EventInstance* pInstance);
  void UpdateOcclusion();

  /// Called when the event resource has been unloaded (for a reload)
  void InvalidateResource(bool bTryToRestore);


  bool m_bPaused;
  bool m_bUseOcclusion;
  ezUInt8 m_uiOcclusionThreshold;
  ezUInt8 m_uiOcclusionCollisionLayer;
  float m_fPitch;
  float m_fVolume;
  ezInt32 m_iTimelinePosition = -1; // used to restore a sound after reloading the resource
  ezUInt32 m_uiOcclusionStateIndex = ezInvalidIndex;
  ezFmodSoundEventResourceHandle m_hSoundEvent;

  FMOD::Studio::EventDescription* m_pEventDesc;
  FMOD::Studio::EventInstance* m_pEventInstance;

  ezEventMessageSender<ezMsgFmodSoundFinished> m_SoundFinishedEventSender; // [ event ]
};
