#include <PCH.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_CREATE_SIMPLE_TEST(Time, Timestamp)
{
  const ezInt64 iFirstContactUnixTimeInSeconds = 2942956800LL;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructors / Valid Check")
  {
    ezTimestamp invalidTimestamp;
    EZ_TEST_BOOL(!invalidTimestamp.IsValid());

    ezTimestamp validTimestamp(0, ezSIUnitOfTime::Second);
    EZ_TEST_BOOL(validTimestamp.IsValid());
    validTimestamp.Invalidate();
    EZ_TEST_BOOL(!validTimestamp.IsValid());

    ezTimestamp currentTimestamp = ezTimestamp::CurrentTimestamp();   
    // Kind of hard to hit a moving target, let's just test if it is in a probable range.
    EZ_TEST_BOOL(currentTimestamp.IsValid());
    EZ_TEST_BOOL_MSG(currentTimestamp.GetInt64(ezSIUnitOfTime::Second) > 1384597970LL, "The current time is before this test was written!");
    EZ_TEST_BOOL_MSG(currentTimestamp.GetInt64(ezSIUnitOfTime::Second) < 32531209845LL, "This current time is after the year 3000! If this is actually the case, please fix this test.");

    // Sleep for 10 milliseconds
    ezThreadUtils::Sleep(10);
    EZ_TEST_BOOL_MSG(currentTimestamp.GetInt64(ezSIUnitOfTime::Microsecond) < ezTimestamp::CurrentTimestamp().GetInt64(ezSIUnitOfTime::Microsecond), "Sleeping for 10 ms should cause the timestamp to change!");
    EZ_TEST_BOOL_MSG(!currentTimestamp.IsEqual(ezTimestamp::CurrentTimestamp(), ezTimestamp::CompareMode::Identical), "Sleeping for 10 ms should cause the timestamp to change!");

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Public Accessors")
  {   
    const ezTimestamp epoch(0, ezSIUnitOfTime::Second);
    const ezTimestamp firstContact(iFirstContactUnixTimeInSeconds, ezSIUnitOfTime::Second);
    EZ_TEST_BOOL(epoch.IsValid());
    EZ_TEST_BOOL(firstContact.IsValid());

    // GetInt64 / SetInt64
    ezTimestamp firstContactTest(iFirstContactUnixTimeInSeconds, ezSIUnitOfTime::Second);
    EZ_TEST_BOOL(firstContactTest.GetInt64(ezSIUnitOfTime::Second) == iFirstContactUnixTimeInSeconds);
    EZ_TEST_BOOL(firstContactTest.GetInt64(ezSIUnitOfTime::Millisecond) == iFirstContactUnixTimeInSeconds * 1000LL);
    EZ_TEST_BOOL(firstContactTest.GetInt64(ezSIUnitOfTime::Microsecond) == iFirstContactUnixTimeInSeconds * 1000000LL);
    EZ_TEST_BOOL(firstContactTest.GetInt64(ezSIUnitOfTime::Nanosecond) == iFirstContactUnixTimeInSeconds * 1000000000LL);

    firstContactTest.SetInt64(firstContactTest.GetInt64(ezSIUnitOfTime::Second), ezSIUnitOfTime::Second);
    EZ_TEST_BOOL(firstContactTest.IsEqual(firstContact, ezTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(ezSIUnitOfTime::Millisecond), ezSIUnitOfTime::Millisecond);
    EZ_TEST_BOOL(firstContactTest.IsEqual(firstContact, ezTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(ezSIUnitOfTime::Microsecond), ezSIUnitOfTime::Microsecond);
    EZ_TEST_BOOL(firstContactTest.IsEqual(firstContact, ezTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(ezSIUnitOfTime::Nanosecond), ezSIUnitOfTime::Nanosecond);
    EZ_TEST_BOOL(firstContactTest.IsEqual(firstContact, ezTimestamp::CompareMode::Identical));

    // IsEqual
    const ezTimestamp firstContactPlusAFewMicroseconds(firstContact.GetInt64(ezSIUnitOfTime::Microsecond) + 42, ezSIUnitOfTime::Microsecond);
    EZ_TEST_BOOL(firstContact.IsEqual(firstContactPlusAFewMicroseconds, ezTimestamp::CompareMode::FileTime));
    EZ_TEST_BOOL(!firstContact.IsEqual(firstContactPlusAFewMicroseconds, ezTimestamp::CompareMode::Identical));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    const ezTimestamp firstContact(iFirstContactUnixTimeInSeconds, ezSIUnitOfTime::Second);

    // Time span arithmetics
    const ezTime timeSpan1000s = ezTime::Seconds(1000);
    EZ_TEST_BOOL(timeSpan1000s.GetMicroseconds() == 1000000000LL);

    // operator +
    const ezTimestamp firstContactPlus1000s = firstContact + timeSpan1000s;
    ezInt64 iSpanDiff = firstContactPlus1000s.GetInt64(ezSIUnitOfTime::Microsecond) - firstContact.GetInt64(ezSIUnitOfTime::Microsecond);
    EZ_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    EZ_TEST_BOOL(firstContactPlus1000s - firstContact == timeSpan1000s);

    const ezTimestamp T1000sPlusFirstContact = timeSpan1000s + firstContact;
    iSpanDiff = T1000sPlusFirstContact.GetInt64(ezSIUnitOfTime::Microsecond) - firstContact.GetInt64(ezSIUnitOfTime::Microsecond);
    EZ_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    EZ_TEST_BOOL(T1000sPlusFirstContact - firstContact == timeSpan1000s);

    // operator -
    const ezTimestamp firstContactMinus1000s = firstContact - timeSpan1000s;
    iSpanDiff = firstContactMinus1000s.GetInt64(ezSIUnitOfTime::Microsecond) - firstContact.GetInt64(ezSIUnitOfTime::Microsecond);
    EZ_TEST_BOOL(iSpanDiff == -1000000000LL);
    // You can only subtract points in time
    EZ_TEST_BOOL(firstContact - firstContactMinus1000s == timeSpan1000s);


    // operator += / -=
    ezTimestamp testTimestamp = firstContact;
    testTimestamp += timeSpan1000s;
    EZ_TEST_BOOL(testTimestamp.IsEqual(firstContactPlus1000s, ezTimestamp::CompareMode::Identical));
    testTimestamp -= timeSpan1000s;
    EZ_TEST_BOOL(testTimestamp.IsEqual(firstContact, ezTimestamp::CompareMode::Identical));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezDateTime conversion")
  {
    // Constructor
    ezDateTime invalidDateTime;
    EZ_TEST_BOOL(!invalidDateTime.GetTimestamp().IsValid());

    const ezTimestamp firstContact(iFirstContactUnixTimeInSeconds, ezSIUnitOfTime::Second);
    ezDateTime firstContactDataTime(firstContact);

    // Getter
    EZ_TEST_INT(firstContactDataTime.GetYear(), 2063);
    EZ_TEST_INT(firstContactDataTime.GetMonth(), 4);
    EZ_TEST_INT(firstContactDataTime.GetDay(), 5);
    EZ_TEST_INT(firstContactDataTime.GetHour(), 0);
    EZ_TEST_INT(firstContactDataTime.GetMinute(), 0);
    EZ_TEST_INT(firstContactDataTime.GetSecond(), 0);
    EZ_TEST_INT(firstContactDataTime.GetMicroseconds(), 0);

    // SetTimestamp / GetTimestamp
    ezTimestamp currentTimestamp = ezTimestamp::CurrentTimestamp();
    ezDateTime currentDateTime;
    currentDateTime.SetTimestamp(currentTimestamp);
    ezTimestamp currentTimestamp2 = currentDateTime.GetTimestamp();
    // OS date time functions should be accurate within one second.
    ezInt64 iDiff = ezMath::Abs(currentTimestamp.GetInt64(ezSIUnitOfTime::Microsecond) - currentTimestamp2.GetInt64(ezSIUnitOfTime::Microsecond));
    EZ_TEST_BOOL(iDiff <= 1000000);

    // Setter
    ezDateTime oneSmallStep;
    oneSmallStep.SetYear(1969);
    oneSmallStep.SetMonth(7);
    oneSmallStep.SetDay(21);
    oneSmallStep.SetHour(2);
    oneSmallStep.SetMinute(56);
    oneSmallStep.SetSecond(0);
    oneSmallStep.SetMicroseconds(0);

    ezTimestamp oneSmallStepTimestamp = oneSmallStep.GetTimestamp();
    EZ_TEST_BOOL(oneSmallStepTimestamp.IsValid());
    EZ_TEST_INT(oneSmallStepTimestamp.GetInt64(ezSIUnitOfTime::Second), -14159040LL);
  }
}

