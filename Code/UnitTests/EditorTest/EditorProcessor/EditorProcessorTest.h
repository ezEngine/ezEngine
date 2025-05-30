#pragma once

#include <EditorTest/EditorTestPCH.h>
#include <Foundation/System/Process.h>

class ezEditorTestEditorProcessor : public ezTestBaseClass
{
public:
  using SUPER = ezTestBaseClass;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    CompileOnly,
    CompileAndTransform
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezTestAppRun TestCompileOnly();
  ezTestAppRun TestCompileAndTransform();

  ezString GetEditorProcessorPath() const;
  ezString GetPacManProjectPath() const;
  ezResult RunEditorProcessor(const ezDynamicArray<ezString>& arguments);

  ezResult PrepareCompile(ezStringBuilder& dllPath, ezStringBuilder& bundlePath);
};
