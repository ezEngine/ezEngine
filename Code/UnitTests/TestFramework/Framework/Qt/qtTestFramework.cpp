#include <TestFramework/TestFrameworkPCH.h>

#ifdef EZ_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#    include <combaseapi.h>
#  endif

////////////////////////////////////////////////////////////////////////
// ezQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

ezQtTestFramework::ezQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : ezTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  // Why this is needed: We use the DirectXTex library which calls GetWICFactory to create its factory singleton. If CoUninitialize is called, this pointer is deleted and another access would crash the application. To prevent this, we take the first reference to CoInitializeEx to ensure that nobody else can init+deinit COM and subsequently corrupt the DirectXTex library. As we init+deinit qt for each test, the first test that does an image comparison will init GetWICFactory and the qt deinit would destroy the WICFactory pointer. The next test that uses image comparison would then trigger the crash.
  HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  EZ_ASSERT_DEV(SUCCEEDED(res), "CoInitializeEx failed with: {}", ezArgErrorCode(res));
#  endif

  Q_INIT_RESOURCE(resources);
  Initialize();
}

ezQtTestFramework::~ezQtTestFramework()
{
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  CoUninitialize();
#  endif
}


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
