#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

/// Creates projects through 'ezEditorProcessor -createProject' and transforms the result.
///
/// Everything runs in a separate process, because creating a project means loading a different set of
/// plugins, which an already running editor cannot do. The transform sub-test is what notices a project
/// template that stopped working - those are otherwise only exercised when someone creates a project by hand.
class ezEditorTestProjectCreation : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateBlankProject,
    ST_CreateBasicFpsProject,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  /// Absolute path of a folder below the test output directory.
  void GetTargetFolder(ezStringBuilder& out_sPath, ezStringView sName) const;
  /// Same, but deletes the folder if it exists, so that a project is always created from scratch.
  ezResult PrepareTargetFolder(ezStringBuilder& out_sPath, ezStringView sName);

  ezStatus CreateBlankProject();
  ezStatus CreateProjectFromTemplate(ezStringView sTemplate);
  ezStatus TransformTemplateProject(ezStringView sTemplate);

  ezString m_sTemplateProjectPath;
};
