#pragma once

#include <Foundation/Time/Time.h>

struct ezSIUnitOfTime
{
  enum Enum
  {
    Nanosecond,     ///< SI-unit of time (10^-9 second)
    Microsecond,    ///< SI-unit of time (10^-6 second)
    Millisecond,    ///< SI-unit of time (10^-3 second)
    Second,         ///< SI-unit of time (base unit)
  };
};

/// \brief The timestamp class encapsulates a date in time as microseconds since Unix epoch.
///
/// The value is represented by an ezInt64 and allows storing time stamps from roughly
/// -291030 BC to 293970 AC.
/// Use this class to efficiently store a timestamp that is valid across platforms.
class EZ_FOUNDATION_DLL ezTimestamp
{
public:
  struct CompareMode
  {
    enum Enum
    {
      FileTime,  ///< Uses a resolution that guarantees that a file's timestamp is considered equal on all platforms.
      Identical, ///< Uses maximal stored resolution.
    };
  };
  /// \brief  Returns the current timestamp. Returned value will always be valid.
  ///
  /// Depending on the platform the precision varies between seconds and nanoseconds.
  static const ezTimestamp CurrentTimestamp(); // [tested]

  EZ_DECLARE_POD_TYPE();

// *** Constructors ***
public:
  /// \brief Creates an invalidated timestamp.
  ezTimestamp(); // [tested]

    /// \brief Creates an new timestamp with the given time in the given unit of time since Unix epoch.
  ezTimestamp(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime); // [tested]

// *** Public Functions ***
public:
  /// \brief Invalidates the timestamp.
  void Invalidate(); // [tested]

  /// \brief Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// \brief Returns the number of 'unitOfTime' since Unix epoch.
  ezInt64 GetInt64(ezSIUnitOfTime::Enum unitOfTime) const; // [tested]

  /// \brief Sets the timestamp as 'iTimeValue' in 'unitOfTime' since Unix epoch.
  void SetInt64(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime); // [tested]

  /// \brief Returns whether this timestamp is considered equal to 'rhs' in the given mode.
  ///
  /// Use CompareMode::FileTime when working with file time stamps across platforms.
  /// It will use the lowest resolution supported by all platforms to make sure the
  /// timestamp of a file is considered equal regardless on which platform it was retrieved.
  bool IsEqual(const ezTimestamp& rhs, CompareMode::Enum mode) const; // [tested]

// *** Operators ***
public:

  /// \brief Adds the time value of "timeSpan" to this data value.
  void operator += (const ezTime& timeSpan); // [tested]

  /// \brief Subtracts the time value of "timeSpan" from this date value.
  void operator -= (const ezTime& timeSpan); // [tested]

  /// \brief Returns the time span between this timestamp and "other".
  const ezTime operator - (const ezTimestamp& other) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the future from this timestamp.
  const ezTimestamp operator + (const ezTime& timeSpan) const; // [tested]

  /// \brief Returns a timestamp that is "timeSpan" further into the past from this timestamp.
  const ezTimestamp operator - (const ezTime& timeSpan) const; // [tested]


private:
  /// \brief The date is stored as microseconds since Unix epoch.
  ezInt64 m_iTimestamp;
};
 
/// \brief Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const ezTimestamp operator+ (ezTime& timeSpan, const ezTimestamp& timestamp);


/// \brief The ezDateTime class can be used to convert ezTimestamp into a human readable form.
///
/// Note: As ezTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class EZ_FOUNDATION_DLL ezDateTime
{
public:
  /// \brief Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  ezDateTime(); // [tested]

  /// \brief Creates a date time instance from the given timestamp.
  ezDateTime(ezTimestamp timestamp); // [tested]

  /// \brief Converts this instance' values into a ezTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the 
  /// not so distant future should be safe.
  const ezTimestamp GetTimestamp() const; // [tested]

  /// \brief Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range,
  /// in which case false will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  bool SetTimestamp(ezTimestamp timestamp); // [tested]

  // *** Accessors ***
public:
  /// \brief Returns the currently set year.
  ezUInt32 GetYear() const; // [tested]

  /// \brief Sets the year to the given value.
  void SetYear(ezInt16 iYear); // [tested]

  /// \brief Returns the currently set month.
  ezUInt8 GetMonth() const; // [tested]

  /// \brief Sets the month to the given value, will be clamped to valid range [1, 12].
  void SetMonth(ezUInt8 uiMonth); // [tested]

  /// \brief Returns the currently set day.
  ezUInt8 GetDay() const; // [tested]

  /// \brief Sets the day to the given value, will be clamped to valid range [1, 31].
  void SetDay(ezUInt8 uiDay); // [tested]

  /// \brief Returns the currently set hour.
  ezUInt8 GetHour() const; // [tested]

  /// \brief Sets the hour to the given value, will be clamped to valid range [0, 23].
  void SetHour(ezUInt8 uiHour); // [tested]

  /// \brief Returns the currently set minute.
  ezUInt8 GetMinute() const; // [tested]

  /// \brief Sets the minute to the given value, will be clamped to valid range [0, 59].
  void SetMinute(ezUInt8 uiMinute); // [tested]

  /// \brief Returns the currently set second.
  ezUInt8 GetSecond() const; // [tested]

  /// \brief Sets the second to the given value, will be clamped to valid range [0, 59].
  void SetSecond(ezUInt8 uiSecond); // [tested]

  /// \brief Returns the currently set microseconds.
  ezUInt32 GetMicroseconds() const; // [tested]

  /// \brief Sets the microseconds to the given value, will be clamped to valid range [0, 999999].
  void SetMicroseconds(ezUInt32 uiMicroSeconds); // [tested]

private:
  /// \brief The fraction of a second in microseconds of this date [0, 999999].
  ezUInt32 m_uiMicroseconds;
  /// \brief The year of this date [-32k, +32k].
  ezInt16 m_iYear;
  /// \brief The month of this date [1, 12].
  ezUInt8 m_uiMonth;
  /// \brief The day of this date [1, 31].
  ezUInt8 m_uiDay;
  /// \brief The hour of this date [0, 23].
  ezUInt8 m_uiHour;
  /// \brief The number of minutes of this date [0, 59].
  ezUInt8 m_uiMinute;
  /// \brief The number of seconds of this date [0, 59].
  ezUInt8 m_uiSecond;
};

#include <Foundation/Time/Implementation/Timestamp_inl.h>

