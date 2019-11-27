#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/Resource.h>
#include <FmodPlugin/Components/FmodComponent.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct ezFmodParameterId
{
public:
  EZ_ALWAYS_INLINE void Invalidate() { m_uiValue = -1; }
  EZ_ALWAYS_INLINE bool IsInvalidated() const { return m_uiValue == -1; }

private:
  ezUInt64 m_uiValue = -1;
};

class ezPhysicsWorldModuleInterface;

class ezFmodEventComponentManager : public ezComponentManager<class ezFmodEventComponent, ezBlockStorageType::FreeList>
{
public:
  ezFmodEventComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezFmodEventComponent;

  struct OcclusionState;
  ezDynamicArray<OcclusionState> m_OcclusionStates;

  ezUInt32 AddOcclusionState(ezFmodEventComponent* pComponent, ezFmodParameterId occlusionParamId, float fRadius);
  void RemoveOcclusionState(ezUInt32 uiIndex);
  const OcclusionState& GetOcclusionState(ezUInt32 uiIndex) const { return m_OcclusionStates[uiIndex]; }

  void ShootOcclusionRays(OcclusionState& state, ezVec3 listenerPos, ezUInt32 uiNumRays, const ezPhysicsWorldModuleInterface* pPhysicsWorldModule, ezTime deltaTime);
  void UpdateOcclusion(const ezWorldModule::UpdateContext& context);
  void UpdateEvents(const ezWorldModule::UpdateContext& context);

  void ResourceEventHandler(const ezResourceEvent& e);
};

typedef ezTypedResourceHandle<class ezFmodSoundEventResource> ezFmodSoundEventResourceHandle;

struct ezResourceEvent;

//////////////////////////////////////////////////////////////////////////

/// \brief Sent when a ezFmodEventComponent finishes playing a sound. Not sent for one-shot sound events.
struct EZ_FMODPLUGIN_DLL ezMsgFmodSoundFinished : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgFmodSoundFinished, ezEventMessage);
};


//////////////////////////////////////////////////////////////////////////

/// \brief Represents a sound (called an 'event') in the Fmod sound system.
///
/// Provides functions to start, pause, stop a sound, set parameters, change volume, pitch etc.
class EZ_FMODPLUGIN_DLL ezFmodEventComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodEventComponent, ezFmodComponent, ezFmodEventComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

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

  void SetPaused(bool b);                      // [ property ]
  bool GetPaused() const { return m_bPaused; } // [ property ]

  void SetUseOcclusion(bool b);                            // [ property ]
  bool GetUseOcclusion() const { return m_bUseOcclusion; } // [ property ]

  void SetOcclusionCollisionLayer(ezUInt8 uiCollisionLayer);                         // [ property ]
  ezUInt8 GetOcclusionCollisionLayer() const { return m_uiOcclusionCollisionLayer; } // [ property ]

  void SetOcclusionThreshold(float fThreshold); // [ property ]
  float GetOcclusionThreshold() const;          // [ property ]

  void SetPitch(float f);                     // [ property ]
  float GetPitch() const { return m_fPitch; } // [ property ]

  void SetVolume(float f);                      // [ property ]
  float GetVolume() const { return m_fVolume; } // [ property ]

  void SetSoundEventFile(const char* szFile); // [ property ]
  const char* GetSoundEventFile() const;      // [ property ]

  void SetSoundEvent(const ezFmodSoundEventResourceHandle& hSoundEvent);
  const ezFmodSoundEventResourceHandle& GetSoundEvent() const { return m_hSoundEvent; }

  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// \brief Will start the sound, if it was not playing. Will restart the sound, if it was already playing.
  /// If the sound was paused so far, this will change the paused state to playing.
  void Restart(); // [ scriptable ]

  /// \brief Plays a completely new sound at the location of this component and with all its current properties.
  ///
  /// Pitch, volume, position, direction and velocity are copied to the new sound instance.
  /// The new sound event then plays to the end and cannot be controlled through this component any further.
  /// If the referenced fmod sound event is not a "one shot" event, this function is ignored.
  /// The event that is controlled through this component is unaffected by this.
  void StartOneShot(); // [ scriptable ]

  /// Stops the current sound from playing. Typically allows the sound to fade out briefly, unless specified otherwise.
  void StopSound(bool bImmediate); // [ scriptable ]

  /// \brief Triggers an fmod sound cue. Whatever that is useful for.
  void SoundCue(); // [ scriptable ]

  /// \brief Tries to find the fmod event parameter by name. Returns the parameter id or -1, if no such parameter exists.
  ezFmodParameterId FindParameter(const char* szName) const;

  /// \brief Sets an fmod event parameter value. See FindParameter() for the index.
  void SetParameter(ezFmodParameterId ParamId, float fValue);

  /// \brief Gets an fmod event parameter value. See FindParameter() for the index. Returns 0, if the index is invalid.
  float GetParameter(ezFmodParameterId ParamId) const;

  /// \brief Sets an event parameter via name lookup, so this is less efficient than SetParameter()
  void SetEventParameter(const char* szParamName, float fValue); // [ scriptable ]

  /// \brief Allows one to set event parameters through the generic ezMsgSetFloatParameter message.
  ///
  /// Requires event parameter lookup via a name, so this is less efficient than SetParameter().
  void OnMsgSetFloatParameter(ezMsgSetFloatParameter& msg); // [ msg handler ]

protected:
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg); // [ msg handler ]

  void Update();
  void UpdateParameters();
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
