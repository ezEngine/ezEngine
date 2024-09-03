#include <TestFramework/TestFrameworkPCH.h>

#ifdef EZ_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

////////////////////////////////////////////////////////////////////////
// ezQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezQtTestFramework::ezQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : ezTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
  Q_INIT_RESOURCE(resources);
  Initialize();
}

ezQtTestFramework::~ezQtTestFramework() = default;


////////////////////////////////////////////////////////////////////////
// ezQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void ezQtTestFramework::OutputImpl(ezTestOutput::Enum Type, const char* szMsg)
{
  ezTestFramework::OutputImpl(Type, szMsg);
}

void ezQtTestFramework::TestResultImpl(ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  ezTestFramework::TestResultImpl(uiSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}


void ezQtTestFramework::SetSubTestStatusImpl(ezUInt32 uiSubTestIndex, const char* szStatus)
{
  ezTestFramework::SetSubTestStatusImpl(uiSubTestIndex, szStatus);
  Q_EMIT TestResultReceived(m_uiCurrentTestIndex, uiSubTestIndex);
}

#endif


