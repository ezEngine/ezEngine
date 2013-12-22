#include <TestFramework/PCH.h>

#ifdef EZ_USE_QT
#include <TestFramework/Framework/Qt/qtTestFramework.h>

////////////////////////////////////////////////////////////////////////
// ezQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezQtTestFramework::ezQtTestFramework(const char* szTestName, const char* szAbsTestDir) : ezTestFramework(szTestName, szAbsTestDir)
{
  Q_INIT_RESOURCE(resources);
}

ezQtTestFramework::~ezQtTestFramework()
{
}


////////////////////////////////////////////////////////////////////////
// ezQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void ezQtTestFramework::OutputImpl(ezTestOutput::Enum Type, const char* szMsg)
{
  ezTestFramework::OutputImpl(Type, szMsg);
}

void ezQtTestFramework::TestResultImpl(ezInt32 iSubTestIdentifier, bool bSuccess, double fDuration)
{
  ezTestFramework::TestResultImpl(iSubTestIdentifier, bSuccess, fDuration);
  emit TestResultReceived(m_iCurrentTestIndex, SubTestIdentifierToSubTestIndex(iSubTestIdentifier));
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestFramework);

