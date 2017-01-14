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
  constexpr static ezTime Nanoseconds(double fNanoseconds)    { return ezTime(fNanoseconds * 0.000000001); }

  /// \brief Creates an instance of ezTime that was initialized from microseconds.
  constexpr static ezTime Microseconds(double fMicroseconds)  { return ezTime(fMicroseconds * 0.000001); }

  /// \brief Creates an instance of ezTime that was initialized from milliseconds.
  constexpr static ezTime Milliseconds(double fMilliseconds)  { return ezTime(fMilliseconds * 0.001); }

  /// \brief Creates an instance of ezTime that was initialized from seconds.
  constexpr static ezTime Seconds(double fSeconds)            { return ezTime(fSeconds); }

  /// \brief Creates an instance of ezTime that was initialized with zero.
  constexpr static ezTime Zero() { return ezTime(0.0); }

  EZ_DECLARE_POD_TYPE();

  /// \brief The default constructor sets the time to zero.
  constexpr ezTime() : m_fTime(0.0) { }

  /// \brief Sets the time value to zero.
  void SetZero();

  /// \brief Returns true if the stored time is zero or negative.
  constexpr bool IsZeroOrLess() const { return m_fTime <= 0.0; }

  /// \brief Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  constexpr float AsFloat() const;

  /// \brief Returns the nanoseconds value
  constexpr double GetNanoseconds() const;

  /// \brief Returns the microseconds value
  constexpr double GetMicroseconds() const;

  /// \brief Returns the milliseconds value
  constexpr double GetMilliseconds() const;

  /// \brief Returns the seconds value.
  constexpr double GetSeconds() const;

  /// \brief Subtracts the time value of "other" from this instances value.
  void operator -= (const ezTime& other);

  /// \brief Adds the time value of "other" to this instances value.
  void operator += (const ezTime& other);

  /// \brief Returns the difference: "this instance - other"
  constexpr ezTime operator - (const ezTime& other) const;

  /// \brief Returns the sum: "this instance + other"
  constexpr ezTime operator + (const ezTime& other) const;

  constexpr bool operator< (const ezTime& rhs) const { return m_fTime <  rhs.m_fTime; }
  constexpr bool operator<=(const ezTime& rhs) const { return m_fTime <= rhs.m_fTime; }
  constexpr bool operator> (const ezTime& rhs) const { return m_fTime >  rhs.m_fTime; }
  constexpr bool operator>=(const ezTime& rhs) const { return m_fTime >= rhs.m_fTime; }
  constexpr bool operator==(const ezTime& rhs) const { return m_fTime == rhs.m_fTime; }
  constexpr bool operator!=(const ezTime& rhs) const { return m_fTime != rhs.m_fTime; }

private:

  /// \brief For internal use only.
  constexpr explicit ezTime(double fTime);

  /// \brief The time is stored in seconds
  double m_fTime;

private:

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

constexpr ezTime operator* (ezTime t, double f);
constexpr ezTime operator* (double f, ezTime t);

constexpr ezTime operator/ (ezTime t, double f);
constexpr ezTime operator/ (double f, ezTime t);


#include <Foundation/Time/Implementation/Time_inl.h>

