#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class ezDocument;

class ezEditorTestGenerateCompile : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

protected:
  ezStatus PrepareCompile(ezStringBuilder& dllPath);
  ezString GetEditorProcessorPath() const;
  ezStatus RunEditorProcessor(const ezDynamicArray<ezString>& arguments);

  ezStatus GenerateAndCompile();
  ezStatus EditorProcessorCompileOnly();
  ezStatus EditorProcessorCompileAndTransform();

  ezString m_sProjectName;
  ezDocument* m_pDocument = nullptr;
};

class ezEditorTestGenerateCompilePacMan : public ezEditorTestGenerateCompile
{
public:
  using SUPER = ezEditorTestGenerateCompile;

  ezEditorTestGenerateCompilePacMan()
  {
    m_sProjectName = "PacMan";
  }

  virtual const char* GetTestName() const override;

private:
  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  enum SubTests
  {
    ST_GenerateAndCompile,
    ST_EditorProcessorCompileOnly,
    ST_EditorProcessorCompileAndTransform
  };
};
