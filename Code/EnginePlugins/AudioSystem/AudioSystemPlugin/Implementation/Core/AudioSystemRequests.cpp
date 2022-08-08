#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Foundation/Types/VariantTypeRegistry.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequest, ezNoBase, 1, ezRTTIDefaultAllocator<ezAudioSystemRequest>)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestRegisterEntity);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestRegisterEntity, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestRegisterEntity>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestRegisterEntity& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_sName;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestRegisterEntity& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_sName;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestUnregisterEntity);
EZ_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(ezAudioSystemRequestUnregisterEntity);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestUnregisterEntity, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestUnregisterEntity>)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestRegisterListener);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestRegisterListener, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestRegisterListener>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestRegisterListener& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_sName;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestRegisterListener& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_sName;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestUnregisterListener);
EZ_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(ezAudioSystemRequestUnregisterListener);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestUnregisterListener, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestUnregisterListener>)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestSetListenerTransform);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestSetListenerTransform, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestSetListenerTransform>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestSetListenerTransform& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_vPosition;
  Stream << Value.m_vForward;
  Stream << Value.m_vUp;
  Stream << Value.m_vVelocity;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestSetListenerTransform& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_vPosition;
  Stream >> Value.m_vForward;
  Stream >> Value.m_vUp;
  Stream >> Value.m_vVelocity;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestLoadTrigger);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestLoadTrigger, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestLoadTrigger>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestLoadTrigger& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_uiEventId;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestLoadTrigger& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_uiEventId;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestActivateTrigger);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestActivateTrigger, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestActivateTrigger>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestActivateTrigger& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_uiEventId;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestActivateTrigger& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_uiEventId;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestStopEvent);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestStopEvent, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestStopEvent>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestStopEvent& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_uiTriggerId;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestStopEvent& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_uiTriggerId;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestSetRtpcValue);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestSetRtpcValue, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestSetRtpcValue>)
EZ_END_STATIC_REFLECTED_TYPE;
void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequestSetRtpcValue& Value)
{
  Stream << static_cast<const ezAudioSystemRequest&>(Value);
  Stream << Value.m_fValue;
}
void operator>>(ezStreamReader& Stream, ezAudioSystemRequestSetRtpcValue& Value)
{
  Stream >> static_cast<ezAudioSystemRequest&>(Value);
  Stream >> Value.m_fValue;
}

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestUnloadTrigger);
EZ_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(ezAudioSystemRequestUnloadTrigger);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestUnloadTrigger, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestUnloadTrigger>)
EZ_END_STATIC_REFLECTED_TYPE;

EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezAudioSystemRequestShutdown);
EZ_DEFINE_AUDIOSYSTEM_STREAM_OPERATORS_SIMPLE(ezAudioSystemRequestShutdown);
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAudioSystemRequestShutdown, ezAudioSystemRequest, 1, ezRTTIDefaultAllocator<ezAudioSystemRequestShutdown>)
EZ_END_STATIC_REFLECTED_TYPE;

void operator<<(ezStreamWriter& Stream, const ezAudioSystemRequest& Value)
{
  Stream << Value.m_eStatus.Succeeded();
  Stream << Value.m_uiEntityId;
  Stream << Value.m_uiListenerId;
  Stream << Value.m_uiObjectId;
}

void operator>>(ezStreamReader& Stream, ezAudioSystemRequest& Value)
{
  bool succeed;
  Stream >> succeed;
  Stream >> Value.m_uiEntityId;
  Stream >> Value.m_uiListenerId;
  Stream >> Value.m_uiObjectId;

  Value.m_eStatus.m_Result = succeed ? EZ_SUCCESS : EZ_FAILURE;
}
