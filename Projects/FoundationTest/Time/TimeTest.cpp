#include <PCH.h>
#include <Foundation/Time/Time.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Time);

EZ_CREATE_SIMPLE_TEST(Time, Timer)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    ezTime TestTime = ezTime::Now();
    
    EZ_TEST_BOOL(TestTime.GetMicroseconds() > 0.0);
    
    volatile ezUInt32 testValue = 0;
    for (ezUInt32 i = 0; i < 42000; ++i)
    {
      testValue += 23;
    }
    
    ezTime TestTime2 = ezTime::Now();
    
    EZ_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);
    
    TestTime2 -= TestTime;
    
    EZ_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);
  }
}

