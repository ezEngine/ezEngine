#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

#include <Foundation/Communication/Message.h>

struct EZ_AUDIOSYSTEMPLUGIN_DLL ezMsgAudioSystemSetRtpcValue : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAudioSystemSetRtpcValue, ezMessage);

  float m_fValue{0.0f};
};

struct EZ_AUDIOSYSTEMPLUGIN_DLL ezMsgAudioSystemSetSwitchState : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAudioSystemSetSwitchState, ezMessage);

  ezAudioSystemDataID m_uiStateId{0};
};

struct EZ_AUDIOSYSTEMPLUGIN_DLL ezMsgAudioSystemSetEnvironmentAmount : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAudioSystemSetEnvironmentAmount, ezMessage);

  float m_fAmount{0.0f};
};
