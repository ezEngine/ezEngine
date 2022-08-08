#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>

typedef ezUInt64 ezAudioSystemDataID;
typedef ezUInt64 ezAudioSystemControlID;

/// \brief The type of a control. This is used by control assets to determine the type of the control
/// when the audio system is parsing them.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemControlType
{
  typedef ezUInt8 StorageType;

  enum Enum : StorageType
  {
    /// \brief The control is not known to the audio system.
    Invalid = 0,

    /// \brief The control is a source.
    Trigger = 1,

    /// \brief The control is a real-time parameter.
    Rtpc = 2,

    /// \brief The control is a sound bank.
    SoundBank = 3,

    /// \brief The control is a switch container.
    Switch = 4,

    /// \brief The control is a switch state.
    SwitchState = 5,

    /// \brief The control is an environment effect.
    Environment = 6,

    Default = Invalid,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, ezAudioSystemControlType);

/// \brief The obstruction type applied to a sound. This affects the way that
/// ray casting works for an audio source.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemSoundObstructionType
{
  typedef ezUInt8 StorageType;

  enum Enum : StorageType
  {
    /// \brief No ray casting is done for this sound.
    /// The sound is neither obstructed nor occluded.
    None = 0,

    /// \brief Only one ray is shot at each frame.
    /// The occlusion value will be averaged over time.
    /// The sound will not be obstructed, since only one ray is not enough
    /// to compute this information.
    SingleRay,

    /// \brief Multiple rays are shot at each frame.
    /// The occlusion and obstructions values will be averaged over time.
    MultipleRay,

    Default = SingleRay
  };

  EZ_ENUM_TO_STRING(None, SingleRay, MultipleRay);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, ezAudioSystemSoundObstructionType);

/// \brief The state of an audio trigger.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemTriggerState
{
  typedef ezUInt8 StorageType;

  enum Enum : StorageType
  {
    /// \brief The trigger have no state, this means it has not been loaded yet
    /// by the audio middleware.
    Invalid = 0,

    /// \brief The trigger is activated and currently playing an event.
    Playing = 1,
    
    /// \brief The trigger is ready to be activated. This state is set after the
    /// trigger is loaded through LoadTrigger.
    Ready = 2,
    
    /// \brief The trigger is being loaded.
    Loading = 3,

    /// \brief The trigger is being unloaded.
    Unloading = 4,

    /// \brief The trigger is being activated.
    Starting = 5,

    /// \brief The trigger is being stopped.
    Stopping = 6,

    /// \brief The trigger is stopped, and not playing an event.
    Stopped = 7,

    Default = Invalid,
  };

  EZ_ENUM_TO_STRING(Invalid, Playing, Ready, Loading, Unloading, Starting, Stopping, Stopped);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, ezAudioSystemTriggerState);

/// \brief The state of an audio source.
enum class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemEventState : ezUInt8
{
  /// \brief The event have no state, this means it has not been loaded yet,
  /// nor triggered by any trigger.
  Invalid = 0,

  /// \brief The event is currently playing audio.
  Playing = 1,
  
  /// \brief The event is loading.
  Loading = 2,
  
  /// \brief The event is being unloaded.
  Unloading = 3,
};

/// \brief Base class for an audio middleware entity.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemEntityData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemEntityData, ezReflectedClass);

public:
  ~ezAudioSystemEntityData() override = default;
};

/// \brief Base class for an audio middleware listener.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemListenerData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemListenerData, ezReflectedClass);

public:
  virtual ~ezAudioSystemListenerData() = default;
};

/// \brief Base class for an audio middleware trigger.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemTriggerData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemTriggerData, ezReflectedClass);

public:
  ~ezAudioSystemTriggerData() override = default;
};

/// \brief Base class for an audio middleware RTPC.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRtpcData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemRtpcData, ezReflectedClass);

public:
  virtual ~ezAudioSystemRtpcData() = default;
};

/// \brief Base class for an audio middleware switch state.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemSwitchStateData
{
  virtual ~ezAudioSystemSwitchStateData() = default;
};

/// \brief Base class for an audio middleware environment.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemEnvironmentData
{
  virtual ~ezAudioSystemEnvironmentData() = default;
};

/// \brief Base class for an audio middleware event.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemEventData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemEventData, ezReflectedClass);

  virtual ~ezAudioSystemEventData() = default;
};

/// \brief Base class for an audio middleware source.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemSourceData
{
  virtual ~ezAudioSystemSourceData() = default;
};

/// \brief Base class for an audio middleware bank file.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemBankData
{
  virtual ~ezAudioSystemBankData() = default;
};
