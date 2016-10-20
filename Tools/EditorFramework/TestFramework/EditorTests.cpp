#include <EditorFramework/TestFramework/EditorTests.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <TestFramework/Framework/TestFramework.h>

void ezQtEditorApp::ExecuteTests()
{
  if (m_TestFramework == nullptr)
  {
    m_TestFramework = EZ_DEFAULT_NEW(ezEditorTests);
  }

  m_TestFramework->ShowTests();
}

ezEditorTests::ezEditorTests()
{
}

ezEditorTests::~ezEditorTests()
{

}

void ezEditorTests::ShowTests()
{
  if (m_TestFramework == nullptr)
  {
    ezStringBuilder sTestsDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sTestsDir.Append("Tests");

    const char* argv = "";
    m_TestFramework = EZ_DEFAULT_NEW(ezTestFramework, "Editor Tests", sTestsDir, 0, &argv);
  }

  m_TestFramework->RunTestExecutionLoop();
}

ezEditorTest::ezEditorTest()
{
}

ezResult ezEditorTest::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTest::GetImage(ezImage& img)
{
  return EZ_FAILURE;
}

static ezBasicEditorTests g_BasicEditorTest;


ezTestAppRun ezBasicEditorTests::RunSubTest(ezInt32 iIdentifier)
{
  if (iIdentifier == SubTests::MyFirstTest)
    return SubtestMyFirstTest();

  return ezTestAppRun::Quit;
}
ezResult ezBasicEditorTests::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezBasicEditorTests::DeInitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezTestAppRun ezBasicEditorTests::SubtestMyFirstTest()
{
  return ezTestAppRun::Quit;
}
