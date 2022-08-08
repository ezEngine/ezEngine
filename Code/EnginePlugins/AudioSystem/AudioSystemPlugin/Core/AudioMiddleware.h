#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Time/Time.h>

/// \brief The Audio Middleware Interface.
/// This interface should be implemented by the ATL middleware to communicate with the audio system.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioMiddleware
{
public:
  // Audio Middleware Interface

  virtual ~ezAudioMiddleware() = default;

  /// \brief Saves the middleware-specific configuration in the ATL config file.
  /// \param writer The DDL file writer.
  virtual ezResult SaveConfiguration(ezOpenDdlWriter& writer) = 0;

  /// \brief Loads the middleware-specif configuration from the ATL config file.
  /// \param reader The DDL file reader.
  virtual ezResult LoadConfiguration(const ezOpenDdlReaderElement& reader) = 0;

  /// \brief Initializes the audio middleware.
  /// \return EZ_SUCCESS when the audio middleware was successfully initialized,
  /// EZ_FAILURE otherwise.
  virtual ezResult Startup() = 0;

  /// \brief Updates the audio middleware.
  /// \param delta The elapsed time since the last update.
  virtual void Update(ezTime delta) = 0;

  /// \brief Deinitializes and stop the audio middleware.
  /// \return EZ_SUCCESS when the audio middleware was successfully stopped,
  /// EZ_FAILURE otherwise.
  virtual ezResult Shutdown() = 0;

  /// \brief Destroys all the resources allocated by the audio middleware.
  /// This is usually called after Shutdown.
  /// \return EZ_SUCCESS when audio middleware resources was successfully released,
  /// EZ_FAILURE otherwise.
  virtual ezResult Release() = 0;

  /// \brief Stops all the sounds actually played by the audio middleware.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult StopAllSounds() = 0;

  /// \brief Add/register an entity in the audio middleware.
  /// This is needed in order to provide transformation values (position and orientation), execute
  /// triggers (play sounds), and set real-time parameters or switches.
  /// \param pEntityData The entity that should be added in the audio middleware.
  /// \param szEntityName The name of the game object representing that entity. (Can be used for debug purposes)
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult AddEntity(ezAudioSystemEntityData* pEntityData, const char* szEntityName) = 0;

  /// \brief Resets an entity state.
  /// \param pEntityData The entity that should be reset.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult ResetEntity(ezAudioSystemEntityData* pEntityData) = 0;

  /// \brief Updates an entity state.
  /// \param pEntityData The entity that should be updated.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult UpdateEntity(ezAudioSystemEntityData* pEntityData) = 0;

  /// \brief Remove/unregister an entity from the audio middleware.
  /// This action disable the possibility to execute triggers, set real-time parameters or switches, and
  /// update the position of the entity.
  /// \param pEntityData The entity that should be removed from the audio middleware.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult RemoveEntity(ezAudioSystemEntityData* pEntityData) = 0;

  /// \brief Sets the global position (world position) of an entity.
  /// \param pEntityData The entity on which set the global position.
  /// \param vPosition The global position of the entity.
  /// \param vForward The forward direction of the entity.
  /// \param vUp The up direction of the entity.
  /// \param vVelocity The velocity of the entity.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetEntityTransform(ezAudioSystemEntityData* pEntityData, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) = 0;

  /// \brief Loads a trigger for a further activation.
  /// All the data and media needed by the trigger will be loaded. Once done, the trigger status will
  /// change to Ready.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult LoadTrigger(ezAudioSystemEntityData* pEntityData, const ezAudioSystemTriggerData* pTriggerData, ezAudioSystemEventData* pEventData) = 0;

  /// \brief Triggers an event on an entity. A trigger is everything which can affect the state of an event.
  /// \param pEntityData The entity on which trigger the event.
  /// \param pTriggerData The event trigger. Can't be modified here.
  /// \param pEventData The triggered event.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult ActivateTrigger(ezAudioSystemEntityData* pEntityData, const ezAudioSystemTriggerData* pTriggerData, ezAudioSystemEventData* pEventData) = 0;

  /// \brief Unload the trigger. This is called when the trigger and all data loaded during LoadTrigger
  /// need to be disposed.
  /// \param pEntityData The entity on which trigger the event.
  /// \param pTriggerData The event trigger. Can't be modified here.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult UnloadTrigger(ezAudioSystemEntityData* pEntityData, const ezAudioSystemTriggerData* pTriggerData) = 0;

  /// \brief Stops an event on the given entity.
  /// \param pEntityData The entity on which stop the event.
  /// \param pEventData The event to stop.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult StopEvent(ezAudioSystemEntityData* pEntityData, const ezAudioSystemEventData* pEventData) = 0;

  /// \brief Stops all events active on the given entity.
  /// \param pEntityData The entity on which stop all events.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult StopAllEvents(ezAudioSystemEntityData* pEntityData) = 0;

  /// \brief Sets an audio RTPC to the specified value on an entity.
  /// \param pEntityData The entity on which set the rtpc value.
  /// \param pRtpcData The rtpc data.
  /// \param fValue The rtpc value.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetRtpc(ezAudioSystemEntityData* pEntityData, const ezAudioSystemRtpcData* pRtpcData, float fValue) = 0;

  /// \brief Resets an audio RTPC to the default value on an entity.
  /// \param pEntityData The entity on which reset the rtpc value.
  /// \param pRtpcData The rtpc data.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult ResetRtpc(ezAudioSystemEntityData* pEntityData, const ezAudioSystemRtpcData* pRtpcData) = 0;

  /// \brief Sets an audio switch to the specified state on an entity.
  /// \param pEntityData The entity on which set the switch value.
  /// \param pSwitchStateData The switch state data.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetSwitchState(ezAudioSystemEntityData* pEntityData, const ezAudioSystemSwitchStateData* pSwitchStateData) = 0;

  /// \brief Sets the obstruction and occlusion values on an entity.
  /// \param pEntityData The entity on which set the obstruction and occlusion values.
  /// \param fObstruction The obstruction value.
  /// \param fOcclusion The occlusion value.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetObstructionAndOcclusion(ezAudioSystemEntityData* pEntityData, float fObstruction, float fOcclusion) = 0;

  /// \brief Sets the amount of an audio environment effect associated on an entity.
  /// \param pEntityData The entity on which set the environment effect value.
  /// \param pEnvironmentData The environment effect data.
  /// \param fAmount The environment effect value.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetEnvironmentAmount(ezAudioSystemEntityData* pEntityData, const ezAudioSystemEnvironmentData* pEnvironmentData, float fAmount) = 0;

  /// \brief Add/register a listener in the audio middleware.
  /// This is needed to let the middleware know where to render audio, and to provide
  /// transformation values (position and orientation).
  /// \param pListenerData The listener that should be added in the audio middleware.
  /// \param szListenerName The name of the game object representing that listener. (Can be used for debug purposes)
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult AddListener(ezAudioSystemListenerData* pListenerData, const char* szListenerName) = 0;

  /// \brief Resets a listener state.
  /// \param pListenerData The listener that should be reset.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult ResetListener(ezAudioSystemListenerData* pListenerData) = 0;

  /// \brief Remove/unregister a listener from the audio middleware.
  /// This action disable the possibility to update the position of the listener.
  /// \param pListenerData The listener that should be removed from the audio middleware.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult RemoveListener(ezAudioSystemListenerData* pListenerData) = 0;

  /// \brief Sets the global transform (wold position and orientation) of a listener.
  /// \param pListenerData The listener data.
  /// \param vPosition The global position of the listener.
  /// \param vForward The forward direction of the listener.
  /// \param vUp The up direction of the listener.
  /// \param vVelocity The velocity of the listener.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetListenerTransform(ezAudioSystemListenerData* pListenerData, const ezVec3& vPosition, const ezVec3& vForward, const ezVec3& vUp, const ezVec3& vVelocity) = 0;

  /// \brief Loads a bank file.
  /// \param pBankData The bank data used for loading.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult LoadBank(ezAudioSystemBankData* pBankData) = 0;

  /// \brief Unloads a bank file.
  /// \param pBankData The bank data used for unloading.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult UnloadBank(ezAudioSystemBankData* pBankData) = 0;

  /// \brief Creates a world entity. This is a special entity used by non-spatial sounds.
  /// The engine will take care to create a game object at position (0,0,0) and orientation (0,0,0,1),
  /// and pass the game object ID as parameter to this function. The game object is ensured to never
  /// move nor rotate.
  /// \param uiEntityId The game object ID referencing the new audio entity.
  /// \return The created entity data, or nullptr if it was not created.
  virtual ezAudioSystemEntityData* CreateWorldEntity(ezAudioSystemDataID uiEntityId) = 0;

  /// \brief Creates an audio entity that is attached to a game object.
  /// \param uiEntityId The game object ID referencing the new audio entity.
  /// \return The created entity data, or nullptr if it was not created.
  virtual ezAudioSystemEntityData* CreateEntityData(ezAudioSystemDataID uiEntityId) = 0;

  /// \brief Destroys an audio entity and release memory.
  /// \param pEntityData The entity data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyEntityData(ezAudioSystemEntityData* pEntityData) = 0;

  /// \brief Creates an audio listener that is attached to a game object.
  /// \param uiListenerId The game object ID referencing the new audio listener.
  /// \return The created listener data, or nullptr if it was not created.
  virtual ezAudioSystemListenerData* CreateListenerData(ezAudioSystemDataID uiListenerId) = 0;

  /// \brief Destroys an audio listener and release memory.
  /// \param pListenerData The listener data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyListenerData(ezAudioSystemListenerData* pListenerData) = 0;

  /// \brief Creates an audio event that is attached to a game object.
  /// \param uiEventId The game object ID referencing the new audio event.
  /// \return The created event data, or nullptr if it was not created.
  virtual ezAudioSystemEventData* CreateEventData(ezAudioSystemDataID uiEventId) = 0;

  /// \brief Resets the audio event state, so it can safely recycled in the pool.
  /// \param pEventData The event data to reset.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult ResetEventData(ezAudioSystemEventData* pEventData) = 0;

  /// \brief Destroys an audio event and release memory.
  /// \param pEventData The event data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyEventData(ezAudioSystemEventData* pEventData) = 0;

  /// \brief Parses the implementation-specific entry that represent a bank.
  /// It's to the audi middleware to fill the struct with required data needed to locate or to store
  /// the file in memory. This data will be further used by LoadBank to load the bank.
  /// \param pBankEntry The stream storing the bank entry. Serialization/deserialization of that stream is implementation specific.
  /// \return The created bank data, or nullptr if no bank was created.
  virtual ezAudioSystemBankData* DeserializeBankEntry(ezStreamReader* pBankEntry) = 0;

  /// \brief Destroys an audio bank and release memory.
  /// \param pBankData The bank data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyBank(ezAudioSystemBankData* pBankData) = 0;

  /// \brief Parses the implementation-specific entry that represent a trigger.
  /// \param pTriggerEntry The stream storing the event entry. Serialization/deserialization of that stream is implementation specific.
  /// \return The created trigger data, or nullptr if no trigger was created.
  virtual ezAudioSystemTriggerData* DeserializeTriggerEntry(ezStreamReader* pTriggerEntry) = 0;

  /// \brief Destroys an audio trigger and release memory.
  /// \param pTriggerData The trigger data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyTriggerData(ezAudioSystemTriggerData* pTriggerData) = 0;

  /// \brief Parses the implementation-specific entry that represent a rtpc.
  /// \param pRtpcEntry The stream storing the rtpc entry. Serialization/deserialization of that stream is implementation specific.
  /// \return The created rtpc data, or nullptr if no rtpc was created.
  virtual ezAudioSystemRtpcData* DeserializeRtpcEntry(ezStreamReader* pRtpcEntry) = 0;

  /// \brief Destroys an audio rtpc and release memory.
  /// \param pRtpcData The rtpc data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyRtpcData(ezAudioSystemRtpcData* pRtpcData) = 0;

  /// \brief Parses the implementation-specific entry that represent a switch state.
  /// \param pSwitchStateEntry The stream storing the switch state entry. Serialization/deserialization of that stream is implementation specific.
  /// \return The created switch state data, or nullptr if no switch state was created.
  virtual ezAudioSystemSwitchStateData* DeserializeSwitchStateEntry(ezStreamReader* pSwitchStateEntry) = 0;

  /// \brief Destroys an audio switch state and release memory.
  /// \param pSwitchStateData The switch state data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroySwitchStateData(ezAudioSystemSwitchStateData* pSwitchStateData) = 0;

  /// \brief Parses the implementation-specific entry that represent a environment effect.
  /// \param pEnvironmentEntry The stream storing the environment effect entry. Serialization/deserialization of that stream is implementation specific.
  /// \return The created environment effect data, or nullptr if no environment effect was created.
  virtual ezAudioSystemEnvironmentData* DeserializeEnvironmentEntry(ezStreamReader* pEnvironmentEntry) = 0;

  /// \brief Destroys an audio environment effect and release memory.
  /// \param pEnvironmentData The environment effect data to destroy.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult DestroyEnvironmentData(ezAudioSystemEnvironmentData* pEnvironmentData) = 0;

  /// \brief Sets the language used by the audio middleware.
  /// \param szLanguage The language to use.
  /// \return EZ_SUCCESS when the operation is successful, EZ_FAILURE otherwise.
  virtual ezResult SetLanguage(const char* szLanguage) = 0;

  /// \brief Gets the audio middleware implementation name.
  /// e.g. "FMOD", "Wwise", "Amplitude", etc.
  /// \return The name of the audio middleware.
  [[nodiscard]] virtual const char* GetMiddlewareName() const = 0;

  /// \brief Gets the name of the directory in which store middleware-specific ATL controls.
  /// It's recommended that the folder name is unique per implementation. It's in that folder
  /// that every ATL controls created and configured for this middleware in the Editor will be stored.
  /// The folder will be located in Sounds/ATL/_folder_name_
  /// \return The middleware folder name.
  [[nodiscard]] virtual const char* GetMiddlewareFolderName() const = 0;

  // Message Handlers

  /// \brief Called each time the game application window loses focus.
  virtual void OnLoseFocus() = 0;

  /// \brief Called each time the game application window gains focus.
  virtual void OnGainFocus() = 0;

  /// \brief Called when the audio middleware should be muted and stop
  /// rendering audio.
  virtual void OnMuteAudio() = 0;

  /// \brief Called when the audio middleware should restart to render audio.
  virtual void OnUnmuteAudio() = 0;
};
