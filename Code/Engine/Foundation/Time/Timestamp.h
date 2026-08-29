#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

struct ezSIUnitOfTime
{
  enum Enum
  {
    Nanosecond,  ///< SI-unit of time (10^-9 second)
    Microsecond, ///< SI-unit of time (10^-6 second)
    Millisecond, ///< SI-unit of time (10^-3 second)
    Second,      ///< SI-unit of time (base unit)
  };
};

/// The timestamp class encapsulates a date in time as microseconds since Unix epoch.
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
      FileTimeEqual, ///< Uses a resolution that guarantees that a file's timestamp is considered equal on all platforms.
      Identical,     ///< Uses maximal stored resolution.
      Newer,         ///< Just compares values and returns true if the left-hand side is larger than the right hand side
    };
  };
  ///  Returns the current timestamp. Returned value will always be valid.
  ///
  /// Depending on the platform the precision varies between seconds and nanoseconds.
  static const ezTimestamp CurrentTimestamp(); // [tested]

  EZ_DECLARE_POD_TYPE();

  // *** Constructors ***
public:
  /// Creates an invalidated timestamp.
  ezTimestamp(); // [tested]

  /// Returns an invalid timestamp
  [[nodiscard]] static ezTimestamp MakeInvalid() { return ezTimestamp(); }

  /// Returns a timestamp initialized from 'iTimeValue' in 'unitOfTime' since Unix epoch.
  [[nodiscard]] static ezTimestamp MakeFromInt(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime);

  // *** Public Functions ***
public:
  /// Returns whether the timestamp is valid.
  bool IsValid() const; // [tested]

  /// Returns the number of 'unitOfTime' since Unix epoch.
  ezInt64 GetInt64(ezSIUnitOfTime::Enum unitOfTime) const; // [tested]

  /// Returns whether this timestamp is considered equal to 'rhs' in the given mode.
  ///
  /// Use CompareMode::FileTime when working with file time stamps across platforms.
  /// It will use the lowest resolution supported by all platforms to make sure the
  /// timestamp of a file is considered equal regardless on which platform it was retrieved.
  bool Compare(const ezTimestamp& rhs, CompareMode::Enum mode) const; // [tested]

  // *** Operators ***
public:
  /// Adds the time value of "timeSpan" to this data value.
  void operator+=(const ezTime& timeSpan); // [tested]

  /// Subtracts the time value of "timeSpan" from this date value.
  void operator-=(const ezTime& timeSpan); // [tested]

  /// Returns the time span between this timestamp and "other".
  const ezTime operator-(const ezTimestamp& other) const; // [tested]

  /// Returns a timestamp that is "timeSpan" further into the future from this timestamp.
  const ezTimestamp operator+(const ezTime& timeSpan) const; // [tested]

  /// Returns a timestamp that is "timeSpan" further into the past from this timestamp.
  const ezTimestamp operator-(const ezTime& timeSpan) const; // [tested]


private:
  static constexpr const ezInt64 EZ_INVALID_TIME_STAMP = ezMath::MinValue<ezInt64>();

  EZ_ALLOW_PRIVATE_PROPERTIES(ezTimestamp);
  /// The date is stored as microseconds since Unix epoch.
  ezInt64 m_iTimestamp = EZ_INVALID_TIME_STAMP;
};

/// Returns a timestamp that is "timeSpan" further into the future from "timestamp".
const ezTimestamp operator+(ezTime& ref_timeSpan, const ezTimestamp& timestamp);

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTimestamp);

/// The ezDateTime class can be used to convert ezTimestamp into a human readable form.
///
/// Note: As ezTimestamp is microseconds since Unix epoch, the values in this class will always be
/// in UTC.
class EZ_FOUNDATION_DLL ezDateTime
{
public:
  /// Creates an empty date time instance with an invalid date.
  ///
  /// Day, Month and Year will be invalid and must be set.
  ezDateTime(); // [tested]
  ~ezDateTime();

  /// Checks whether all values are within valid ranges.
  bool IsValid() const;

  /// Returns a date time that is all zero.
  [[nodiscard]] static ezDateTime MakeZero() { return ezDateTime(); }

  /// Sets this instance to the given timestamp.
  ///
  /// This calls SetFromTimestamp() internally and asserts that the conversion succeeded.
  /// Use SetFromTimestamp() directly, if you need to be able to react to invalid data.
  [[nodiscard]] static ezDateTime MakeFromTimestamp(ezTimestamp timestamp);

  /// Converts this instance' values into a ezTimestamp.
  ///
  /// The conversion is done via the OS and can fail for values that are outside the supported range.
  /// In this case, the returned value will be invalid. Anything after 1970 and before the
  /// not so distant future should be safe.
  [[nodiscard]] const ezTimestamp GetTimestamp() const; // [tested]

  /// Sets this instance to the given timestamp.
  ///
  /// The conversion is done via the OS and will fail for invalid dates and values outside the supported range,
  /// in which case EZ_FAILURE will be returned.
  /// Anything after 1970 and before the not so distant future should be safe.
  ezResult SetFromTimestamp(ezTimestamp timestamp);

  // *** Accessors ***
public:
  /// Returns the currently set year.
  ezUInt32 GetYear() const; // [tested]

  /// Sets the year to the given value.
  void SetYear(ezInt16 iYear); // [tested]

  /// Returns the currently set month.
  ezUInt8 GetMonth() const; // [tested]

  /// Sets the month to the given value. Asserts that the value is in the valid range [1, 12].
  void SetMonth(ezUInt8 uiMonth); // [tested]

  /// Returns the currently set day.
  ezUInt8 GetDay() const; // [tested]

  /// Sets the day to the given value. Asserts that the value is in the valid range [1, 31].
  void SetDay(ezUInt8 uiDay); // [tested]

  /// Returns the currently set day of week.
  ezUInt8 GetDayOfWeek() const;

  /// Sets the day of week to the given value. Asserts that the value is in the valid range [0, 6].
  void SetDayOfWeek(ezUInt8 uiDayOfWeek);

  /// Returns the currently set hour.
  ezUInt8 GetHour() const; // [tested]

  /// Sets the hour to the given value. Asserts that the value is in the valid range [0, 23].
  void SetHour(ezUInt8 uiHour); // [tested]

  /// Returns the currently set minute.
  ezUInt8 GetMinute() const; // [tested]

  /// Sets the minute to the given value. Asserts that the value is in the valid range [0, 59].
  void SetMinute(ezUInt8 uiMinute); // [tested]

  /// Returns the currently set second.
  ezUInt8 GetSecond() const; // [tested]

  /// Sets the second to the given value. Asserts that the value is in the valid range [0, 59].
  void SetSecond(ezUInt8 uiSecond); // [tested]

  /// Returns the currently set microseconds.
  ezUInt32 GetMicroseconds() const; // [tested]

  /// Sets the microseconds to the given value. Asserts that the value is in the valid range [0, 999999].
  void SetMicroseconds(ezUInt32 uiMicroSeconds); // [tested]

private:
  /// The fraction of a second in microseconds of this date [0, 999999].
  ezUInt32 m_uiMicroseconds = 0;
  /// The year of this date [-32k, +32k].
  ezInt16 m_iYear = 0;
  /// The month of this date [1, 12].
  ezUInt8 m_uiMonth = 0;
  /// The day of this date [1, 31].
  ezUInt8 m_uiDay = 0;
  /// The day of week of this date [0, 6].
  ezUInt8 m_uiDayOfWeek = 0;
  /// The hour of this date [0, 23].
  ezUInt8 m_uiHour = 0;
  /// The number of minutes of this date [0, 59].
  ezUInt8 m_uiMinute = 0;
  /// The number of seconds of this date [0, 59].
  ezUInt8 m_uiSecond = 0;
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezDateTime& arg);

struct ezArgDateTime
{
  enum FormattingFlags
  {
    ShowDate = EZ_BIT(0),
    TextualDate = ShowDate | EZ_BIT(1),
    ShowWeekday = EZ_BIT(2),
    ShowTime = EZ_BIT(3),
    ShowSeconds = ShowTime | EZ_BIT(4),
    ShowMilliseconds = ShowSeconds | EZ_BIT(5),
    ShowTimeZone = EZ_BIT(6),

    Default = ShowDate | ShowSeconds,
    DefaultTextual = TextualDate | ShowSeconds,
  };

  /// Initialized a formatting object for an ezDateTime instance.
  /// \param dateTime The ezDateTime instance to format.
  /// \param bUseNames Indicates whether to use names for days of week and months (true)
  ///        or a purely numerical representation (false).
  /// \param bShowTimeZoneIndicator Whether to indicate the timezone of the ezDateTime object.
  inline explicit ezArgDateTime(const ezDateTime& dateTime, ezUInt32 uiFormattingFlags = Default)
    : m_Value(dateTime)
    , m_uiFormattingFlags(uiFormattingFlags)
  {
  }

  ezDateTime m_Value;
  ezUInt32 m_uiFormattingFlags;
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgDateTime& arg);

#include <Foundation/Time/Implementation/Timestamp_inl.h>
