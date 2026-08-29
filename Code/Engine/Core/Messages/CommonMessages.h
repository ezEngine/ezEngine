#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Color.h>

/// Common message for components that can be toggled between playing and paused states
struct EZ_CORE_DLL ezMsgSetPlaying : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetPlaying, ezMessage);

  bool m_bPlay = true;
};

/// Common message for components that can or need to be canceled immediately
struct EZ_CORE_DLL ezMsgInterruptPlaying : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgInterruptPlaying, ezMessage);
};

/// Basic message to set some generic parameter to a float value.
struct EZ_CORE_DLL ezMsgSetFloatParameter : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetFloatParameter, ezMessage);

  ezString m_sParameterName;
  float m_fValue = 0;
};

/// Basic message to set some generic parameter to a color value.
struct EZ_CORE_DLL ezMsgSetColorParameter : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSetColorParameter, ezMessage);

  ezString m_sParameterName;
  ezColor m_Value = ezColor::White;
};

/// For use in scripts to signal a custom event that some game event has occurred.
///
/// This is a simple message for simple use cases. Create custom messages for more elaborate cases where a string is not sufficient
/// information.
struct EZ_CORE_DLL ezMsgGenericEvent : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgGenericEvent, ezMessage);

  /// A custom string to identify the intent.
  ezHashedString m_sMessage;
  ezVariant m_Value;
};

/// Sent when an animation reached its end (either forwards or backwards playing)
///
/// This is sent regardless of whether the animation is played once, looped or back and forth,
/// ie. it should be sent at each 'end' point, even when it then starts another cycle.
struct EZ_CORE_DLL ezMsgAnimationReachedEnd : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationReachedEnd, ezMessage);
};
