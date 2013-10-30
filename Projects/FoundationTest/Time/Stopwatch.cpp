#include <PCH.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_CREATE_SIMPLE_TEST(Time, Stopwatch)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "General Functionality")
  {
    ezStopwatch sw;

    ezThreadUtils::Sleep(50);

    sw.StopAndReset();
    sw.Resume();

    const ezTime t0 = sw.Checkpoint();

    ezThreadUtils::Sleep(10);

    const ezTime t1 = sw.Checkpoint();

    ezThreadUtils::Sleep(20);

    const ezTime t2 = sw.Checkpoint();

    ezThreadUtils::Sleep(30);

    const ezTime t3 = sw.Checkpoint();

    const ezTime tTotal1 = sw.GetRunningTotal();

    ezThreadUtils::Sleep(10);

    sw.Pause(); // freeze the current running total

    const ezTime tTotal2 = sw.GetRunningTotal();

    ezThreadUtils::Sleep(10); // should not affect the running total anymore

    const ezTime tTotal3 = sw.GetRunningTotal();


    // these tests are deliberately written such that they cannot fail,
    // even when the OS is under heavy load

    EZ_TEST(t0 > ezTime::MilliSeconds(5));
    EZ_TEST(t1 > ezTime::MilliSeconds(5));
    EZ_TEST(t2 > ezTime::MilliSeconds(5));
    EZ_TEST(t3 > ezTime::MilliSeconds(5));


    EZ_TEST(t1 + t2 + t3 <= tTotal1);
    EZ_TEST(t0 + t1 + t2 + t3 > tTotal1);

    EZ_TEST(tTotal1 < tTotal2);
    EZ_TEST(tTotal1 < tTotal3);
    EZ_TEST(tTotal2 == tTotal3);
  }
}

