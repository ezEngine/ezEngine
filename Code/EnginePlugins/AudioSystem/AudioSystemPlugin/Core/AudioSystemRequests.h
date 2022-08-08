#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantType.h>


/// \brief Helper macro the declare the callback value of an audio request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name) \
  using CallbackType = ezDelegate<void(const name&)>; \
  CallbackType m_Callback


/// \brief Helper macro to declare a new audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(name)                                       \
  EZ_DECLARE_POD_TYPE();                                                                       \
  bool operator==(const name& rhs) const                                                       \
  {                                                                                            \
    return static_cast<ezAudioSystemRequest>(*this) == static_cast<ezAudioSystemRequest>(rhs); \
  }                                                                                            \
  bool operator!=(const name& rhs) const                                                       \
  {                                                                                            \
    return !(*this == rhs);                                                                    \
  }                                                                                            \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name)


/// \brief Helper macro to declare a new audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(name, eq_ex)                                                  \
  EZ_DECLARE_POD_TYPE();                                                                                  \
  bool operator==(const name& rhs) const                                                                  \
  {                                                                                                       \
    return static_cast<ezAudioSystemRequest>(*this) == static_cast<ezAudioSystemRequest>(rhs) && (eq_ex); \
  }                                                                                                       \
  bool operator!=(const name& rhs) const                                                                  \
  {                                                                                                       \
    return !(*this == rhs);                                                                               \
  }                                                                                                       \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_CALLBACK(name)


/// \brief Helper macro to declare an ezHashHelper implementation of an audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_HASH_SIMPLE(name)      \
  template <>                                                 \
  struct ezHashHelper<name>                                   \
  {                                                           \
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const name& value)  \
    {                                                         \
      return ezHashHelper<ezAudioSystemRequest>::Hash(value); \
    }                                                         \
    EZ_ALWAYS_INLINE static bool                              \
    Equal(const name& a, const name& b)                       \
    {                                                         \
      return a == b;                                          \
    }                                                         \
  }


/// \brief Helper macro to declare an ezHashHelper implementation of an audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_HASH(name, hash_ex)                \
  template <>                                                             \
  struct ezHashHelper<name>                                               \
  {                                                                       \
    EZ_ALWAYS_INLINE static ezUInt32 Hash(const name& value)              \
    {                                                                     \
      return ezHashHelper<ezAudioSystemRequest>::Hash(value) * (hash_ex); \
    }                                                                     \
    EZ_ALWAYS_INLINE static bool                                          \
    Equal(const name& a, const name& b)                                   \
    {                                                                     \
      return a == b;                                                      \
    }                                                                     \
  }


/// \brief Helper macro to declare stream operators for an audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name)                          \
  EZ_AUDIOSYSTEMPLUGIN_DLL void operator<<(ezStreamWriter& Stream, const name& Value); \
  EZ_AUDIOSYSTEMPLUGIN_DLL void operator>>(ezStreamReader& Stream, name& Value)


/// \brief Helper macro to declare a new audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(name)            \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_HASH_SIMPLE(name);            \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name);       \
  EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, name); \
  EZ_DECLARE_CUSTOM_VARIANT_TYPE(name)


/// \brief Helper macro to declare a new audio system request.
#define EZ_DECLARE_AUDIOSYSTEM_REQUEST(name, hash_ex)          \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_HASH(name, hash_ex);          \
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(name);       \
  EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, name); \
  EZ_DECLARE_CUSTOM_VARIANT_TYPE(name)


/// \brief Helper macro to define stream operators for audio system
/// requests which doesn't add more data.
#define EZ_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(name)    \
  void operator<<(ezStreamWriter& Stream, const name& Value)   \
  {                                                            \
    Stream << static_cast<const ezAudioSystemRequest&>(Value); \
  }                                                            \
  void operator>>(ezStreamReader& Stream, name& Value)         \
  {                                                            \
    Stream >> static_cast<ezAudioSystemRequest&>(Value);       \
  }


/// \brief Base class for all audio system requests.
///
/// An audio request is a message sent to the audio system. It contains a type and a payload.
/// The payload depend on the audio request purpose (load trigger, update listener, shutdown system, etc.).
///
/// To send an audio request, use the ezAudioSystem::SendRequest() function. Audio requests sent this way
/// will be executed asynchronously in the audio thread.
///
/// Each audio requests can have callbacks, this can be useful when you want to do some logic after an
/// asynchronous audio request as been sent to the audio system. The callbacks will be executed in the
/// main thread.
///
/// If you need to send an audio request synchronously, use the ezAudioSystem::SendRequestSync() function.
/// This will block the main thread until the request has been processed.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequest
{
  EZ_DECLARE_POD_TYPE();

  /// \brief The audio entity which is being manipulated, if any.
  ezAudioSystemDataID m_uiEntityId{0};

  /// \brief The audio listener which is being manipulated, if any.
  ezAudioSystemDataID m_uiListenerId{0};

  /// \brief The audio object (trigger, rtpc, environment, etc.) which is being manipulated, if any.
  ezAudioSystemDataID m_uiObjectId{0};

  /// \brief The status of the audio request.
  ezStatus m_eStatus;

  bool operator==(const ezAudioSystemRequest& rhs) const
  {
    return m_uiEntityId == rhs.m_uiEntityId && m_uiListenerId == rhs.m_uiListenerId && m_uiObjectId == rhs.m_uiObjectId && m_eStatus.Succeeded() == rhs.m_eStatus.Succeeded();
  }

  bool operator!=(const ezAudioSystemRequest& rhs) const
  {
    return !(*this == rhs);
  }
};

template <>
struct ezHashHelper<ezAudioSystemRequest>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezAudioSystemRequest& value)
  {
    return ezHashHelper<ezUInt64>::Hash(value.m_uiEntityId) * ezHashHelper<ezUInt64>::Hash(value.m_uiListenerId) * ezHashHelper<ezUInt64>::Hash(value.m_uiObjectId);
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezAudioSystemRequest& a, const ezAudioSystemRequest& b)
  {
    return a == b;
  }
};

/// \brief Audio request to register a new entity in the audio system.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestRegisterEntity : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestRegisterEntity, m_sName == rhs.m_sName);

  /// \brief A friendly name for the entity. Not very used by most of the audio engines for
  /// other purposes than debugging.
  ezString m_sName;
};

/// \brief Audio request to unregister an entity from the audio system.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestUnregisterEntity : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(ezAudioSystemRequestUnregisterEntity);
};

/// \brief Audio request to register a new listener in the audio system.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestRegisterListener : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestRegisterListener, m_sName == rhs.m_sName);

  /// \brief A friendly name for the listener. Not very used by most of the audio engines for
  /// other purposes than debugging.
  ezString m_sName;
};

/// \brief Audio request to unregister a listener from the audio system.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestUnregisterListener : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(ezAudioSystemRequestUnregisterListener);
};

/// \brief Audio request to set the transform and velocity of a listener.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestSetListenerTransform : public ezAudioSystemRequest
{
  // clang-format off
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(
    ezAudioSystemRequestSetListenerTransform,
    m_vPosition == rhs.m_vPosition && m_vForward == rhs.m_vForward && m_vUp == rhs.m_vUp && m_vVelocity == rhs.m_vVelocity
  );
  // clang-format on

  ezVec3 m_vPosition;
  ezVec3 m_vForward;
  ezVec3 m_vUp;
  ezVec3 m_vVelocity;
};

/// \brief Audio request to load data needed to activate a trigger.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestLoadTrigger : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestLoadTrigger, m_uiEventId == rhs.m_uiEventId);

  /// \brief The event that this trigger should start.
  ezAudioSystemDataID m_uiEventId{0};
};

/// \brief Audio request to activate a trigger.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestActivateTrigger : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestActivateTrigger, m_uiEventId == rhs.m_uiEventId);

  /// \brief The event that this trigger should start.
  ezAudioSystemDataID m_uiEventId{0};
};

/// \brief Audio request to stop an event.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestStopEvent : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestStopEvent, m_uiTriggerId == rhs.m_uiTriggerId);

  /// \brief The trigger which have started this event.
  ezAudioSystemDataID m_uiTriggerId{0};
};

/// \brief Audio request to unload data loaded by a trigger.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestUnloadTrigger : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(ezAudioSystemRequestUnloadTrigger);
};

/// \brief Audio request to set the value of a real-time parameter.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestSetRtpcValue : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE(ezAudioSystemRequestSetRtpcValue, m_fValue == rhs.m_fValue);

  /// \brief The new parameter's value.
  float m_fValue{0.0f};
};

/// \brief Audio request to shutdown the audio system. Used internally only. Sending this request
/// at runtime will lead to unspecified behaviors.
struct EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemRequestShutdown : public ezAudioSystemRequest
{
  EZ_DECLARE_AUDIOSYSTEM_REQUEST_TYPE_SIMPLE(ezAudioSystemRequestShutdown);
};

EZ_DECLARE_AUDIOSYSTEM_REQUEST_STREAM_OPERATORS(ezAudioSystemRequest);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_AUDIOSYSTEMPLUGIN_DLL, ezAudioSystemRequest);

EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestRegisterEntity, ezHashHelper<ezString>::Hash(value.m_sName));
EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(ezAudioSystemRequestUnregisterEntity);
EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestRegisterListener, ezHashHelper<ezString>::Hash(value.m_sName));
EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(ezAudioSystemRequestUnregisterListener);
EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(ezAudioSystemRequestSetListenerTransform); // TODO: Hash ezVec3
EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestLoadTrigger, ezHashHelper<ezAudioSystemDataID>::Hash(value.m_uiEventId));
EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestActivateTrigger, ezHashHelper<ezAudioSystemDataID>::Hash(value.m_uiEventId));
EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestStopEvent, ezHashHelper<ezAudioSystemDataID>::Hash(value.m_uiTriggerId));
EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(ezAudioSystemRequestUnloadTrigger);
EZ_DECLARE_AUDIOSYSTEM_REQUEST(ezAudioSystemRequestSetRtpcValue, ezHashHelper<ezInt32>::Hash(ezMath::FloatToInt(value.m_fValue * 1000.0f)));
EZ_DECLARE_AUDIOSYSTEM_REQUEST_SIMPLE(ezAudioSystemRequestShutdown);

/// \brief A functor used by ezVariant to call an audio request callback.
struct CallRequestCallbackFunc
{
  explicit CallRequestCallbackFunc(const ezVariant& value)
    : m_Value(value)
  {
  }

  template <typename T>
  EZ_ALWAYS_INLINE void Call() const
  {
    ezResult conversionStatus = EZ_SUCCESS;
    const auto& request = m_Value.ConvertTo<T>(&conversionStatus);

    if (conversionStatus.Succeeded())
    {
      request.m_Callback(request);
    }
  }

  template <typename T>
  void operator()()
  {
    if (m_Value.IsA<ezAudioSystemRequestRegisterEntity>())
    {
      Call<ezAudioSystemRequestRegisterEntity>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestUnregisterEntity>())
    {
      Call<ezAudioSystemRequestUnregisterEntity>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestLoadTrigger>())
    {
      Call<ezAudioSystemRequestLoadTrigger>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestActivateTrigger>())
    {
      Call<ezAudioSystemRequestActivateTrigger>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestStopEvent>())
    {
      Call<ezAudioSystemRequestStopEvent>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestSetRtpcValue>())
    {
      Call<ezAudioSystemRequestSetRtpcValue>();
    }
    else if (m_Value.IsA<ezAudioSystemRequestShutdown>())
    {
      Call<ezAudioSystemRequestShutdown>();
    }
    else
    {
      ezLog::Error("Received an Audio Request with an invalid type");
    }
  }

  const ezVariant& m_Value;
};
