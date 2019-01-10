#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>

class ezTimeStepSmoothing;

/// \brief A clock that can be speed up, slowed down, paused, etc. Useful for updating game logic, rendering, etc.
class EZ_FOUNDATION_DLL ezClock
{
public:
  /// \brief Returns the global clock.
  static ezClock* GetGlobalClock() { return s_pGlobalClock; }

public:
  /// \brief Constructor.
  ezClock(const char* szName); // [tested]

  /// \brief Resets all values to their default. E.g. call this after a new level has loaded to start fresh.
  ///
  /// If \a bEverything is false, only the current state of the clock is reset (accumulated time, speed, paused).
  /// Otherwise the clock is entirely reset, clearing also the time step smoother, min/max time steps and fixed time step.
  void Reset(bool bEverything); // [tested]

  /// \brief Updates the clock using the time difference since the last call to Update().
  ///
  /// If a fixed time step is set, that will be used as the time difference.
  /// If the timer is paused, the time difference is set to zero.
  /// The time difference will then be scaled and clamped according to the clock speed and minimum and maximum time step.
  void Update(); // [tested]

  /// \brief Sets a time step smoother for this clock. Pass nullptr to deactivate time step smoothing.
  ///
  /// Also calls ezTimeStepSmoothing::Reset() on any non-nullptr pSmoother.
  void SetTimeStepSmoothing(ezTimeStepSmoothing* pSmoother);

  /// \brief Returns the object used for time step smoothing (if any).
  ezTimeStepSmoothing* GetTimeStepSmoothing() const; // [tested]

  /// \brief Sets the clock to be paused or running.
  void SetPaused(bool bPaused); // [tested]

  /// \brief Returns the paused state.
  bool GetPaused() const; // [tested]

  /// \brief Sets a fixed time step for updating the clock.
  ///
  /// If tDiff is set to zero (the default), fixed time stepping is disabled.
  /// Fixed time stepping allows to run the simulation at a constant rate, which is useful
  /// for recording videos or to step subsystems that require constant steps.
  /// Clock speed, pause and min/max time step are still being applied even when the time step is fixed.
  void SetFixedTimeStep(ezTime tDiff = ezTime()); // [tested]

  /// \brief Returns the value for the fixed time step (zero if it is disabled).
  ezTime GetFixedTimeStep() const; // [tested]

  /// \brief Allows to replace the current accumulated time.
  ///
  /// This can be used to reset the time to a specific point, e.g. when a game state is loaded from file,
  /// one should also reset the time to the time that was used when the game state was saved, to ensure
  /// that game objects that stored the accumulated time for reference, will continue to work.
  /// However, prefer to use Save() and Load() as those functions will store and restore the entire clock state.
  void SetAccumulatedTime(ezTime t); // [tested]

  /// \brief Returns the accumulated time since the last call to Reset().
  ///
  /// The accumulated time is basically the 'absolute' time in the game world.
  /// Since this is the accumulation of all scaled, paused and clamped time steps,
  /// it will most likely have no relation to the real time that has passed.
  ezTime GetAccumulatedTime() const; // [tested]

  /// \brief Returns the time difference between the last two calls to Update().
  ///
  /// This is the main function to use to query how much to advance some simulation.
  /// The time step is already scaled, clamped, etc.
  ezTime GetTimeDiff() const; // [tested]

  /// \brief The factor with which to scale the time step during calls to Update().
  void SetSpeed(double fFactor); // [tested]

  /// \brief Returns the clock speed multiplier.
  double GetSpeed() const; // [tested]

  /// \brief Sets the minimum time that must pass between clock updates.
  ///
  /// By default a minimum time step of 0.001 seconds is enabled to ensure that code does not break down
  /// due to very small time steps. The minimum time step is applied after the clock speed is applied.
  /// When a custom time step smoother is set, that class needs to apply the clock speed AND also clamp
  /// the value to the min/max time step (which means it can ignore or override that feature).
  /// When the clock is paused, it will always return a time step of zero.
  void SetMinimumTimeStep(ezTime tMin); // [tested]

  /// \brief Sets the maximum time that may pass between clock updates.
  ///
  /// By default a maximum time step of 0.1 seconds is enabled to ensure that code does not break down
  /// due to very large time steps. The maximum time step is applied after the clock speed is applied.
  /// When a custom time step smoother is set, that class needs to apply the clock speed AND also clamp
  /// the value to the min/max time step (which means it can ignore or override that feature).
  /// \sa SetMinimumTimeStep
  void SetMaximumTimeStep(ezTime tMax); // [tested]

  /// \brief Returns the value for the minimum time step.
  /// \sa SetMinimumTimeStep
  ezTime GetMinimumTimeStep() const; // [tested]

  /// \brief Returns the value for the maximum time step.
  /// \sa SetMaximumTimeStep
  ezTime GetMaximumTimeStep() const; // [tested]

  /// \brief Serializes the current clock state to a stream.
  void Save(ezStreamWriter& Stream) const;

  /// \brief Deserializes the current clock state from a stream.
  void Load(ezStreamReader& Stream);

  /// \brief Sets the name of the clock. Useful to identify the clock in tools such as ezInspector.
  void SetClockName(const char* szName);

  /// \brief Returns the name of the clock. All clocks get default names 'Clock N', unless the user specifies another name with
  /// SetClockName.
  const char* GetClockName() const;


public:
  /// \brief The data that is sent through the event interface.
  struct EventData
  {
    const char* m_szClockName;

    ezTime m_RawTimeStep;
    ezTime m_SmoothedTimeStep;
  };

  typedef ezEvent<const EventData&, ezMutex> Event;

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_TimeEvents.AddEventHandler(handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_TimeEvents.RemoveEventHandler(handler); }


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Clock);

  static Event s_TimeEvents;
  static ezClock* s_pGlobalClock;

  ezString m_sName;

  ezTime m_AccumulatedTime;
  ezTime m_LastTimeDiff;
  ezTime m_FixedTimeStep;
  ezTime m_LastTimeUpdate;
  ezTime m_MinTimeStep;
  ezTime m_MaxTimeStep;

  double m_Speed;
  bool m_bPaused;

  ezTimeStepSmoothing* m_pTimeStepSmoother;
};


/// \brief Base class for all time step smoothing algorithms.
///
/// By deriving from this class you can implement your own algorithms for time step smoothing.
/// Then just set an instance of that class on one of the clocks and it will be applied to the time step.
class EZ_FOUNDATION_DLL ezTimeStepSmoothing
{
public:
  virtual ~ezTimeStepSmoothing() {}

  /// \brief The function to override to implement time step smoothing.
  ///
  /// \param RawTimeStep
  ///   The actual raw time difference since the last clock update without any modification.
  /// \param pClock
  ///   The clock that calls this time step smoother.
  ///   Can be used to look up the clock speed and min/max time step.
  ///
  /// \note It is the responsibility of each ezTimeStepSmoothing class to implement
  /// clock speed and also to clamp the time step to the min/max values.
  /// This allows the smoothing algorithm to override these values, if necessary.
  virtual ezTime GetSmoothedTimeStep(ezTime RawTimeStep, const ezClock* pClock) = 0;

  /// \brief Called when ezClock::Reset(), ezClock::Load() or ezClock::SetPaused(true) was called.
  ///
  /// \param pClock
  ///   The clock that is calling this function.
  virtual void Reset(const ezClock* pClock) = 0;
};



#include <Foundation/Time/Implementation/Clock_inl.h>
