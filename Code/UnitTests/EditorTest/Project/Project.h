#pragma once

#include <EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class ezEditorTestProject : public ezEditorTest
{
public:
  typedef ezEditorTest SUPER;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateDocuments,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

};
