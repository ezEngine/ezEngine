#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../GenerateCompile/GenerateCompile.h"

class ezEditorTestSamples : public ezEditorTestGenerateCompile
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_Asteroids,
    ST_PacMan,
    ST_RTS,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
};
