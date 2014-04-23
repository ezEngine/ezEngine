#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Threading/ThreadUtils.h>

class ezSimpleTimeStepSmoother : public ezTimeStepSmoothing
{
public:
  virtual ezTime GetSmoothedTimeStep(ezTime RawTimeStep, const ezClock* pClock) override
  {
    return ezTime::Seconds(0.42);
  }

  virtual void Reset(const ezClock* pClock) override
  {
  }
};

EZ_CREATE_SIMPLE_TEST(Time, Clock)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor / Reset")
  {
    ezClock c; // calls 'Reset' internally

    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor

    EZ_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 0.0, 0.0);
    EZ_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);
    EZ_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);
    EZ_TEST_BOOL(c.GetPaused() == false);
    EZ_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants
    EZ_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0); // to ensure the tests fail if somebody changes these constants
    EZ_TEST_BOOL(c.GetTimeDiff() > ezTime::Seconds(0.0));

    ezSimpleTimeStepSmoother s;

    c.SetTimeStepSmoothing(&s);

    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(false);

    // does NOT reset which time step smoother to use
    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(true);
    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetPaused / GetPaused")
  {
    ezClock c;
    EZ_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    EZ_TEST_BOOL(c.GetPaused());

    c.SetPaused(false);
    EZ_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    EZ_TEST_BOOL(c.GetPaused());

    c.Reset(false);
    EZ_TEST_BOOL(!c.GetPaused());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Updates while Paused / Unpaused")
  {
    ezClock c;

    c.SetPaused(false);

    const ezTime t0 = c.GetAccumulatedTime();

    ezThreadUtils::Sleep(10);
    c.Update();

    const ezTime t1 = c.GetAccumulatedTime();
    EZ_TEST_BOOL(t0 < t1);

    c.SetPaused(true);

    ezThreadUtils::Sleep(10);
    c.Update();

    const ezTime t2 = c.GetAccumulatedTime();
    EZ_TEST_BOOL(t1 == t2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetFixedTimeStep / GetFixedTimeStep")
  {
    ezClock c;

    EZ_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);

    c.SetFixedTimeStep(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Updates with fixed time step")
  {
    ezClock c;
    c.SetFixedTimeStep(ezTime::Seconds(1.0 / 60.0));
    c.Update();

    ezThreadUtils::Sleep(10);

    c.Update();
    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    ezThreadUtils::Sleep(50);

    c.Update();
    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    c.Update();
    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetAccumulatedTime / GetAccumulatedTime")
  {
    ezClock c;

    c.SetAccumulatedTime(ezTime::Seconds(23.42));

    EZ_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 23.42, 0.000001);

    c.Update(); // by default after a SetAccumulatedTime the time diff should always be > 0

    EZ_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);

    const ezTime t0 = c.GetAccumulatedTime();

    ezThreadUtils::Sleep(5);
    c.Update();

    const ezTime t1 = c.GetAccumulatedTime();

    EZ_TEST_BOOL(t1 > t0);
    EZ_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSpeed / GetSpeed / GetTimeDiff")
  {
    ezClock c;
    EZ_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);

    c.SetFixedTimeStep(ezTime::Seconds(0.01));

    c.SetSpeed(10.0);
    EZ_TEST_DOUBLE(c.GetSpeed(), 10.0, 0.000001);

    c.Update();
    const ezTime t0 = c.GetTimeDiff();
    EZ_TEST_DOUBLE(t0.GetSeconds(), 0.1, 0.00001)

    c.SetSpeed(0.1);

    c.Update();
    const ezTime t1 = c.GetTimeDiff();
    EZ_TEST_DOUBLE(t1.GetSeconds(), 0.001, 0.00001)

    c.Reset(false);

    c.Update();
    const ezTime t2 = c.GetTimeDiff();
    EZ_TEST_DOUBLE(t2.GetSeconds(), 0.01, 0.00001)
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetMinimumTimeStep / GetMinimumTimeStep")
  {
    ezClock c;
    EZ_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants

    c.Update();
    c.Update();

    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMinimumTimeStep(ezTime::Seconds(0.1));
    c.SetMaximumTimeStep(ezTime::Seconds(1.0));

    EZ_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.1, 0.0);

    c.Update();
    c.Update();

    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetMaximumTimeStep / GetMaximumTimeStep")
  {
    ezClock c;
    EZ_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0); // to ensure the tests fail if somebody changes these constants

    ezThreadUtils::Sleep(200);
    c.Update();

    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMaximumTimeStep(ezTime::Seconds(0.2));

    EZ_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.2, 0.0);

    ezThreadUtils::Sleep(400);
    c.Update();

    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetTimeStepSmoothing / GetTimeStepSmoothing")
  {
    ezClock c;

    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr);
    
    ezSimpleTimeStepSmoother s;
    c.SetTimeStepSmoothing(&s);

    EZ_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.SetMaximumTimeStep(ezTime::Seconds(10.0)); // this would limit the time step even after smoothing
    c.Update();

    EZ_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 0.42, 0.0);
  }
}

