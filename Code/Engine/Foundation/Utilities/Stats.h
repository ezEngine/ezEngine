#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \brief This class holds a simple map that maps strings (keys) to strings (values), which represent certain stats.
///
/// This can be used by a game to store (and continuously update) information about the internal game state. Other tools can then
/// display this information in a convenient manner. For example the stats can be shown on screen. The data is also transmitted through
/// ezTelemetry, and the ezInspector tool will display the information.
class EZ_FOUNDATION_DLL ezStats
{
public:
  typedef ezMap<ezString, ezString> MapType;

  /// \brief Removes the stat with the given name.
  ///
  /// This will also send a 'remove' message through ezTelemetry, such that external tools can remove it from their list.
  static void RemoveStat(const char* szStatName);

  /// \brief Sets the value of the given stat, adds it if it did not exist before.
  ///
  /// szStatName may contain slashes (but not backslashes) to define groups and subgroups, which can be used by tools such as ezInspector
  /// to display the stats in a hierarchical way.
  /// This function will also send the name and value of the stat through ezTelemetry, such that tools like ezInspector will show the
  /// changed value.
  static void SetStat(const char* szStatName, const char* szValue);

  /// \brief Returns the value of the given stat. Returns an empty string, if the stat did not exist before.
  static const char* GetStat(const char* szStatName) { return s_Stats[szStatName].GetData(); }

  /// \brief Returns the entire map of stats, can be used to display them.
  static const MapType& GetAllStats() { return s_Stats; }

  /// \brief The event data that is broadcast whenever a stat is changed.
  struct StatsEventData
  {
    /// \brief Which type of event this is.
    enum EventType
    {
      Add,    ///< A variable has been set for the first time.
      Set,    ///< A variable has been changed.
      Remove  ///< A variable that existed has been removed.
    };

    EventType m_EventType;
    const char* m_szStatName;
    const char* m_szNewStatValue;
  };

  typedef ezEvent<const StatsEventData&, ezMutex> ezEventStats;

  /// \brief Adds an event handler that is called every time a stat is changed.
  static void AddEventHandler(ezEventStats::Handler handler)    { s_StatsEvents.AddEventHandler    (handler); }

  /// \brief Removes a previously added event handler.
  static void RemoveEventHandler(ezEventStats::Handler handler) { s_StatsEvents.RemoveEventHandler (handler); }

private:
  static MapType s_Stats;

  static ezEventStats s_StatsEvents;
};

