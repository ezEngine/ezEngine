#include <TestFramework/PCH.h>

EZ_STATICLINK_LIBRARY(TestFramework)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtLogMessageDock);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestDelegate);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestFramework);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestGUI);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_Qt_qtTestModel);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_SimpleTest);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_TestBaseClass);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_TestFramework);
  EZ_STATICLINK_REFERENCE(TestFramework_Framework_TestResults);
  EZ_STATICLINK_REFERENCE(TestFramework_Utilities_TestOrder);
  EZ_STATICLINK_REFERENCE(TestFramework_Utilities_TestSetup);
}

