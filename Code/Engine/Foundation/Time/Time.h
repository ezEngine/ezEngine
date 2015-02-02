#pragma once

#include <Foundation/Basics.h>

/// \brief The time class encapsulates a double value storing the time in seconds.
///
/// It offers convenient functions to get the time in other units.
/// ezTime is a high-precision time using the OS specific high-precision timing functions
/// and may thus be used for profiling as well as simulation code.
class EZ_FOUNDATION_DLL ezTime
{
public:

  /// \brief Gets the current time
  static ezTime Now(); // [tested]

  /// \brief Creates an instance of ezTime that was initialized from nanoseconds.
  static ezTime Nanoseconds(double fNanoseconds)    { return ezTime(fNanoseconds * 0.000000001); }

  /// \brief Creates an instance of ezTime that was initialized from microseconds.
  static ezTime Microseconds(double fMicroseconds)  { return ezTime(fMicroseconds * 0.000001); }

  /// \brief Creates an instance of ezTime that was initialized from milliseconds.
  static ezTime Milliseconds(double fMilliseconds)  { return ezTime(fMilliseconds * 0.001); }

  /// \brief Creates an instance of ezTime that was initialized from seconds.
  static ezTime Seconds(double fSeconds)            { return ezTime(fSeconds); }

  EZ_DECLARE_POD_TYPE();

  /// \brief The default constructor sets the time to zero.
  ezTime() { m_fTime = 0.0; }

  /// \brief Sets the time value to zero.
  void SetZero();
  
  /// \brief Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  float AsFloat() const;

  /// \brief Returns the nanoseconds value
  double GetNanoseconds() const;

  /// \brief Returns the microseconds value
  double GetMicroseconds() const;

  /// \brief Returns the milliseconds value
  double GetMilliseconds() const;

  /// \brief Returns the seconds value.
  double GetSeconds() const;

  /// \brief Subtracts the time value of "other" from this instances value.
  void operator -= (const ezTime& other);

  /// \brief Adds the time value of "other" to this instances value.
  void operator += (const ezTime& other);

  /// \brief Returns the difference: "this instance - other"
  ezTime operator - (const ezTime& other) const;

  /// \brief Returns the sum: "this instance + other"
  ezTime operator + (const ezTime& other) const;

  bool operator< (const ezTime& rhs) const { return m_fTime <  rhs.m_fTime; }
  bool operator<=(const ezTime& rhs) const { return m_fTime <= rhs.m_fTime; }
  bool operator> (const ezTime& rhs) const { return m_fTime >  rhs.m_fTime; }
  bool operator>=(const ezTime& rhs) const { return m_fTime >= rhs.m_fTime; }
  bool operator==(const ezTime& rhs) const { return m_fTime == rhs.m_fTime; }
  bool operator!=(const ezTime& rhs) const { return m_fTime != rhs.m_fTime; }

private:

  /// \brief For internal use only.
  explicit ezTime(double fTime);

  /// \brief The time is stored in seconds
  double m_fTime;

private:

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

ezTime operator* (ezTime t, double f);
ezTime operator* (double f, ezTime t);

ezTime operator/ (ezTime t, double f);
ezTime operator/ (double f, ezTime t);


#include <Foundation/Time/Implementation/Time_inl.h>

