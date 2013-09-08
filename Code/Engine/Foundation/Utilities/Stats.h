#pragma once

#include <Foundation/Basics.h>
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
  typedef ezMap<ezString, ezString, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> MapType;

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

  /// \brief Sends the values and names of ALL stats through ezTelemetry. Does not need to be called by hand,
  ///   it will be called automatically be ezTelemetry (at the moment) to sync stats when a new connection is made).
  static void SendAllStatsTelemetry();

  /// \todo Add an ezEvent for stat changes

private:
  static MapType s_Stats;
};

