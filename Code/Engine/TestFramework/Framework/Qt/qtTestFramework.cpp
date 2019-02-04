#include <PCH.h>

#ifdef EZ_USE_QT
#include <TestFramework/Framework/Qt/qtTestFramework.h>

////////////////////////////////////////////////////////////////////////
// ezQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezQtTestFramework::ezQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc,
                                     const char** argv)
    : ezTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, argc, argv)
{
  Q_INIT_RESOURCE(resources);
  Initialize();
}

ezQtTestFramework::~ezQtTestFramework() {}


////////////////////////////////////////////////////////////////////////
// ezQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void ezQtTestFramework::OutputImpl(ezTestOutput::Enum Type, const char* szMsg)
{
  ezTestFramework::OutputImpl(Type, szMsg);
}

void ezQtTestFramework::TestResultImpl(ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  ezTestFramework::TestResultImpl(iSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_iCurrentTestIndex, iSubTestIndex);
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestFramework);

