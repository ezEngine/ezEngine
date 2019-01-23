#include <PCH.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

ezInt32 ezConstructionCounterRelocatable::s_iConstructions = 0;
ezInt32 ezConstructionCounterRelocatable::s_iDestructions = 0;
ezInt32 ezConstructionCounterRelocatable::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounterRelocatable::s_iDestructionsLast = 0;

EZ_TESTFRAMEWORK_ENTRY_POINT("FoundationTest", "Foundation Tests")
