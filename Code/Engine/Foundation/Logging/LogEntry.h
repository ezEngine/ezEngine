#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezLogMsgType);

/// \brief A persistent log entry created from a ezLoggingEventData.
/// Allows for a log event to survive for longer than just the event
/// and is reflected, allowing for it to be sent to remote targets.
struct EZ_FOUNDATION_DLL ezLogEntry
{
  ezLogEntry();
  ezLogEntry(const ezLoggingEventData& le);

  ezString m_sMsg;
  ezString m_sTag;
  ezEnum<ezLogMsgType> m_Type;
  ezUInt8 m_uiIndentation = 0;
  double m_fSeconds = 0;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezLogEntry);

/// \brief A log interface implementation that converts a log event into
/// a ezLogEntry and calls a delegate with it.
///
/// A typical use case is to re-route and store log messages in a scope:
/// \code{.cpp}
///   {
///     ezLogEntryDelegate logger(([&array](ezLogEntry& entry) -> void
///     {
///       array.PushBack(std::move(entry));
///     }));
///     ezLogSystemScope logScope(&logger);
///     *log something*
///   }
/// \endcode
class EZ_FOUNDATION_DLL ezLogEntryDelegate : public ezLogInterface
{
public:
  typedef ezDelegate<void(ezLogEntry&)> Callback;
  /// \brief Log events will be delegated to the given callback.
  ezLogEntryDelegate(Callback callback, ezLogMsgType::Enum LogLevel = ezLogMsgType::All);
  virtual void HandleLogMessage(const ezLoggingEventData& le) override;

private:
  Callback m_Callback;
};

