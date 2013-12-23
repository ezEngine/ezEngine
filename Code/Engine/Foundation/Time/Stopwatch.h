#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>

/// \brief A helper class to time the execution of code or other actions.
class EZ_FOUNDATION_DLL ezStopwatch
{
public:
  /// \brief The constructor will automatically start the stopwatch.
  ezStopwatch(); // [tested]

  /// \brief Stops the stopwatch and resets the running total to zero. Afterwards a new time measure can be started using Resume().
  void StopAndReset();

  /// \brief Pauses the stopwatch so that the running total will not increase any further.
  ///
  /// The stopwatch can be resumed via Resume() later and will then accumulate more time. The time in between will not contribute to the running total.
  void Pause(); // [tested]

  /// \brief Resumes the stopwatch to accumulate more time.
  ///
  /// If the stopwatch is already running, this function has no effect. To start a completely new time measure,
  /// first call StopAndReset() and then Resume().
  void Resume(); // [tested]

  /// \brief Returns the current running total. This can be called while the stopwatch is running or while it is paused.
  ///
  /// The running total can be frozen by calling Pause(), which allows to read the result back at a later point in time.
  /// It can be reset to zero using StopAndReset().
  ezTime GetRunningTotal(); // [tested]

  /// \brief This function returns the time that has passed since the last call to Checkpoint() or since the construction of the object.
  ///
  /// You can use this function to measure the time between steps. Just call Checkpoint() after each step to retrieve the time it took.
  /// To retrieve the time since the stopwatch started, call GetRunningTotal().
  ///
  /// \note Checkpoint() is not affected by calls to Pause(), Resume() or StopAndReset(). It always returns the time difference to the
  /// last call to Checkpoint().
  ezTime Checkpoint(); // [tested]

private:
  bool m_bRunning;
  ezTime m_LastUpdate;
  ezTime m_LastCheckpoint;
  ezTime m_TotalDuration;
};

