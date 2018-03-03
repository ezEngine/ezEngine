#include <PCH.h>
#include <Foundation/Tracks/EventTrack.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Tracks);

EZ_CREATE_SIMPLE_TEST(Tracks, EventTrack)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty")
  {
    ezEventTrack et;
    ezHybridArray<ezHashedString, 8> result;

    EZ_TEST_BOOL(et.IsEmpty());
    et.Sample(ezTime::Zero(), ezTime::Seconds(1.0), result);

    EZ_TEST_BOOL(result.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sample")
  {
    ezEventTrack et;
    ezHybridArray<ezHashedString, 8> result;

    et.AddControlPoint(ezTime::Seconds(3.0), "Event3");
    et.AddControlPoint(ezTime::Seconds(0.0), "Event0");
    et.AddControlPoint(ezTime::Seconds(4.0), "Event4");
    et.AddControlPoint(ezTime::Seconds(1.0), "Event1");
    et.AddControlPoint(ezTime::Seconds(2.0), "Event2");

    EZ_TEST_BOOL(!et.IsEmpty());

    // sampling an empty range should yield no results, even if sampling an exact time where an event is
    {
      {
        et.Sample(ezTime::Seconds(0.0), ezTime::Seconds(0.0), result);
        EZ_TEST_INT(result.GetCount(), 0);
      }

      {
        et.Sample(ezTime::Seconds(1.0), ezTime::Seconds(1.0), result);
        EZ_TEST_INT(result.GetCount(), 0);
      }

      {
        et.Sample(ezTime::Seconds(4.0), ezTime::Seconds(4.0), result);
        EZ_TEST_INT(result.GetCount(), 0);
      }
    }

    {
      et.Sample(ezTime::Seconds(0.0), ezTime::Seconds(1.0), result);
      EZ_TEST_INT(result.GetCount(), 1);
      EZ_TEST_STRING(result[0].GetString(), "Event0");
    }

    {
      et.Sample(ezTime::Seconds(0.0), ezTime::Seconds(2.0), result);
      EZ_TEST_INT(result.GetCount(), 2);
      EZ_TEST_STRING(result[0].GetString(), "Event0");
      EZ_TEST_STRING(result[1].GetString(), "Event1");
    }

    {
      et.Sample(ezTime::Seconds(0.0), ezTime::Seconds(4.0), result);
      EZ_TEST_INT(result.GetCount(), 4);
      EZ_TEST_STRING(result[0].GetString(), "Event0");
      EZ_TEST_STRING(result[1].GetString(), "Event1");
      EZ_TEST_STRING(result[2].GetString(), "Event2");
      EZ_TEST_STRING(result[3].GetString(), "Event3");
    }

    {
      et.Sample(ezTime::Seconds(0.0), ezTime::Seconds(10.0), result);
      EZ_TEST_INT(result.GetCount(), 5);
      EZ_TEST_STRING(result[0].GetString(), "Event0");
      EZ_TEST_STRING(result[1].GetString(), "Event1");
      EZ_TEST_STRING(result[2].GetString(), "Event2");
      EZ_TEST_STRING(result[3].GetString(), "Event3");
      EZ_TEST_STRING(result[4].GetString(), "Event4");
    }

    {
      et.Sample(ezTime::Seconds(-0.1), ezTime::Seconds(10.0), result);
      EZ_TEST_INT(result.GetCount(), 5);
      EZ_TEST_STRING(result[0].GetString(), "Event0");
      EZ_TEST_STRING(result[1].GetString(), "Event1");
      EZ_TEST_STRING(result[2].GetString(), "Event2");
      EZ_TEST_STRING(result[3].GetString(), "Event3");
      EZ_TEST_STRING(result[4].GetString(), "Event4");
    }

    et.Clear();
    EZ_TEST_BOOL(et.IsEmpty());
  }
}

