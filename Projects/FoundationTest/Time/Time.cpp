#include <PCH.h>
#include <Foundation/Time/Time.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Time);

EZ_CREATE_SIMPLE_TEST(Time, Timer)
{
  EZ_TEST_BLOCK(true, "Basics")
  {
    ezTime TestTime = ezSystemTime::Now();
    
    EZ_TEST(TestTime.GetMicroSeconds() > 0.0);
    
    volatile ezUInt32 testValue = 0;
    for (ezUInt32 i = 0; i < 42000; ++i)
    {
      testValue += 23;
    }
    
    ezTime TestTime2 = ezSystemTime::Now();
    
    EZ_TEST(TestTime2.GetMicroSeconds() > 0.0);
    
    TestTime2 -= TestTime;
    
    EZ_TEST(TestTime2.GetMicroSeconds() > 0.0);
  }
}

