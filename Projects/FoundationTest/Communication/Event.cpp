#include <PCH.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Communication);

void DoStuff(ezInt32* pEventData, ezInt32* pPassThrough)
{
  ezInt32* iPassThrough = (ezInt32*) pPassThrough;

  ezInt32* iData = (ezInt32*) pEventData;
  *iData += *iPassThrough;
}

EZ_CREATE_SIMPLE_TEST(Communication, Event)
{
  {
    ezEvent<ezInt32*, ezInt32*> e;

    ezInt32 iER1 = 3;
    ezInt32 iER2 = 5;
    ezInt32 iResult = 0;

    e.AddEventHandler(DoStuff, &iER1);

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 3);

    e.AddEventHandler(DoStuff, &iER2);

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 8);

    e.RemoveEventHandler(DoStuff, &iER1);

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 5);

    e.RemoveEventHandler(DoStuff, &iER2);

    iResult = 0;
    e.Broadcast(&iResult);

    EZ_TEST_INT(iResult, 0);
  }
}
