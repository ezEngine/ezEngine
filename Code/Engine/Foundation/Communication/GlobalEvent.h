#pragma once

/// \file

#include <Foundation/Types/Variant.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// \brief A class to broadcast and handle global (system-wide) events.
///
/// A global event is an event that will be sent to all instances of ezGlobalEvent (or rather their
/// respective handler functions), without the need to first register these event-handlers anywhere.
/// Thus they are very useful to notify sub-systems of certain important events, such as that some kind of
/// initialization will be done shortly, which means they can react by preparing properly.
/// For example the ezStartup-class will send certain events before doing startup and shutdown steps, which
/// allows code to free resources before a sub-system might be shut down.
/// ezGlobalEvent's should be used when there is a kind of event that should be propagated throughout the entire
/// engine, without knowledge which systems might want to know about it. These systems can then use an
/// ezGlobalEvent-instance to hook themselves into the global-event pipeline and react accordingly.
/// Global events should mostly be used for startup / configuration / shutdown procedures.
/// Also one should never assume any specific order of execution, all event handlers should be completely independent
/// from each other.
///
/// To create a global event handler, simply add this code inside a cpp file:
///
/// EZ_ON_GLOBAL_EVENT(EventName)
/// {
///   ... do something ...
/// }
///
/// You can also use EZ_ON_GLOBAL_EVENT_ONCE, if the handler should only be executed the first time the event is sent.
/// This is more efficient than filtering out duplicate events inside the event handler.
class EZ_FOUNDATION_DLL ezGlobalEvent : public ezEnumerable<ezGlobalEvent>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezGlobalEvent);

public:

  struct EZ_FOUNDATION_DLL EventData
  {
    EventData();

    ezUInt32 m_uiNumTimesFired;
    ezUInt16 m_uiNumEventHandlersRegular;
    ezUInt16 m_uiNumEventHandlersOnce;
  };

  typedef ezMap<ezString, EventData> EventMap;

public:

  /// \brief [internal] Use the macro EZ_ON_GLOBAL_EVENT or EZ_ON_GLOBAL_EVENT_ONCE to create an event handler.
  typedef void (*EZ_GLOBAL_EVENT_HANDLER)(const ezVariant& param0, const ezVariant& param1, const ezVariant& param2, const ezVariant& param3);

  /// \brief [internal] Use the macro EZ_ON_GLOBAL_EVENT or EZ_ON_GLOBAL_EVENT_ONCE to create an event handler.
  ezGlobalEvent(const char* szEventName, EZ_GLOBAL_EVENT_HANDLER EventHandler, bool bOnlyOnce); // [tested]

  /// \brief This function will broadcast a system wide event to all event handlers that are registered to handle this specific type of event.
  ///
  /// The string specifies the event type, the parameters are optional and can be used to send additional event specific data.
  static void Broadcast(const char* szEventName, ezVariant param0 = ezVariant(), ezVariant param1 = ezVariant(), ezVariant param2 = ezVariant(), ezVariant param3 = ezVariant()); // [tested]

  /// \brief This function will output (via ezLog) some statistics about which events are used and how often.
  ///
  /// This allows to figure out which events are used throughout the engine and which events might be fired too often.
  static void PrintGlobalEventStatistics(); // [tested]

  /// \brief Updates all global event statistics.
  static void UpdateGlobalEventStatistics();

  /// \brief Returns the map that holds the current statistics about the global events.
  static const EventMap& GetEventStatistics() { return s_KnownEvents; }

private:
  bool m_bOnlyOnce;
  bool m_bHasBeenFired;
  const char* m_szEventName;
  EZ_GLOBAL_EVENT_HANDLER m_EventHandler;

  static EventMap s_KnownEvents;
};


/// \brief Use this macro to handle an event every time it is broadcast (place function code in curly brackets after it)
#define EZ_ON_GLOBAL_EVENT(name) \
  static void EventHandler_##name (const ezVariant& param0, const ezVariant& param1, const ezVariant& param2, const ezVariant& param3); \
  static ezGlobalEvent s_EventHandler_##name (#name, EventHandler_##name, false); \
  static void EventHandler_##name (const ezVariant& param0, const ezVariant& param1, const ezVariant& param2, const ezVariant& param3)

/// \brief Use this macro to handle an event only once (place function code in curly brackets after it)
#define EZ_ON_GLOBAL_EVENT_ONCE(name) \
  static void EventHandler_##name (const ezVariant& param0, const ezVariant& param1, const ezVariant& param2, const ezVariant& param3); \
  static ezGlobalEvent s_EventHandler_##name (#name, EventHandler_##name, true); \
  static void EventHandler_##name (const ezVariant& param0, const ezVariant& param1, const ezVariant& param2, const ezVariant& param3)



