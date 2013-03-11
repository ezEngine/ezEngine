#pragma once

#include <Foundation/Basics.h>

/// The time class encapsulates a double value storing the time in milliseconds
/// and offers convenient functions to get the time in other units.
/// ezTime is a high-precision time using the OS specific high-precision timing functions
/// and my thus be used for profiling as well as simulation code.
class EZ_FOUNDATION_DLL ezTime
{
public:

  /// Helper class to encapsulate nanoseconds.
  class NanoSeconds
  {
  public:

    explicit NanoSeconds(double fNanoSeconds) : m_fTime(fNanoSeconds) { }

  private:

    double m_fTime;

    friend class ezTime;
  };

  /// Helper class to encapsulate microseconds.
  class MicroSeconds
  {
  public:

    explicit MicroSeconds(double fMicroSeconds) : m_fTime(fMicroSeconds) { }

  private:

    double m_fTime;

    friend class ezTime;
  };

  /// Helper class to encapsulate milliseconds.
  class MilliSeconds
  {
  public:
    
    explicit MilliSeconds(double fMilliSeconds) : m_fTime(fMilliSeconds) { }

  private:

    double m_fTime;

    friend class ezTime;
  };

  /// Helper class to encapsulate seconds.
  class Seconds
  {
  public:

    explicit Seconds(double fSeconds) : m_fTime(fSeconds) { }

  private:

    double m_fTime;

    friend class ezTime;
  };

  ezTime() { m_fTime = 0.0; }

  /// Initializes the ezTime instance with fTime as seconds. For clarity the usage of ezTime::Seconds may be preferred.
  explicit ezTime(double fTime);

  /// Initializes the ezTime instance from a known nanoseconds value.
  explicit ezTime(const NanoSeconds& nanoSeconds);

  /// Initializes the ezTime instance from a known microseconds value.
  explicit ezTime(const MicroSeconds& microSeconds);

  /// Initializes the ezTime instance from a known milliseconds value.
  explicit ezTime(const MilliSeconds& milliSeconds);

  /// Initializes the ezTime instance from a known seconds value.
  explicit ezTime(const Seconds& seconds);
  
  /// Sets the value of the instance from a known nanoseconds value.
  void Set(const NanoSeconds& seconds);

  /// Sets the value of the instance from a known microseconds value.
  void Set(const MicroSeconds& seconds);

  /// Sets the value of the instance from a known milliseconds value.
  void Set(const MilliSeconds& seconds);

  /// Sets the value of the instance from a known econds value.
  void Set(const Seconds& seconds);

  /// Returns the time as a float value (in seconds).
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriote quickly. (Only use for delta times is recommended)
  float AsFloat() const;

  /// Returns the nanoseconds value
  double GetNanoSeconds() const;

  /// Returns the microseconds value
  double GetMicroSeconds() const;

  /// Returns the milliseconds value
  double GetMilliSeconds() const;

  /// Returns the seconds value.
  double GetSeconds() const;

  /// Subtracts the time value of "other" from this instances value.
  void operator -= (const ezTime& other);

  /// Adds the time value of "other" to this instances value.
  void operator += (const ezTime& other);

  /// Returns the difference: "this instance - other"
  ezTime operator - (const ezTime& other) const;

  /// Returns the sum: "this instance + other"
  ezTime operator + (const ezTime& other) const;

private:

  /// The time is stored in seconds
  double m_fTime;
};


/// Encapsulation of functions in relation to system handling of the time.
struct EZ_FOUNDATION_DLL ezSystemTime
{
public:
  
  /// Gets the current time
  static ezTime Now();

private:

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();

  static void Shutdown();
};


#include <Foundation/Time/Implementation/Time_inl.h>

