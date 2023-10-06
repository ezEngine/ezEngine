#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class ezEditorTestProject : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateDocuments,
    ST_CreateCppSolution
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezTestAppRun CreateDocuments();
  ezTestAppRun CreateCppSolution();
};
