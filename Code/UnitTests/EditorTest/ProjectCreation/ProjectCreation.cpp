#include <EditorTest/EditorTestPCH.h>

#include "ProjectCreation.h"
#include <Foundation/IO/OSFile.h>
#include <TestFramework/Framework/TestFramework.h>

static ezEditorTestProjectCreation s_EditorTestProjectCreation;

const char* ezEditorTestProjectCreation::GetTestName() const
{
  return "Project Creation";
}

void ezEditorTestProjectCreation::SetupSubTests()
{
  AddSubTest("01 - Create Blank Project", SubTests::ST_CreateBlankProject);
  AddSubTest("02 - Create 'Basic FPS' Project", SubTests::ST_CreateBasicFpsProject);
}

ezResult ezEditorTestProjectCreation::InitializeTest()
{
  // no project is opened here - every sub-test creates its own in a separate process
  return SUPER::InitializeTest();
}

ezResult ezEditorTestProjectCreation::DeInitializeTest()
{
  return SUPER::DeInitializeTest();
}

ezTestAppRun ezEditorTestProjectCreation::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ezStatus res(EZ_SUCCESS);

  switch (iIdentifier)
  {
    case ST_CreateBlankProject:
      res = CreateBlankProject();
      break;
    case ST_CreateBasicFpsProject:
      res = TransformTemplateProject("Basic FPS");
      break;
    default:
      EZ_REPORT_FAILURE("missing case statement");
      break;
  }

  if (res.Failed())
  {
    ezTestFramework::GetInstance()->Error(res.GetMessageString(), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, "");
  }

  return ezTestAppRun::Quit;
}

void ezEditorTestProjectCreation::GetTargetFolder(ezStringBuilder& out_sPath, ezStringView sName) const
{
  out_sPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  out_sPath.AppendPath(GetTestName(), sName);
  out_sPath.MakeCleanPath();
}

ezResult ezEditorTestProjectCreation::PrepareTargetFolder(ezStringBuilder& out_sPath, ezStringView sName)
{
  GetTargetFolder(out_sPath, sName);

  if (ezOSFile::ExistsDirectory(out_sPath) && ezOSFile::DeleteFolder(out_sPath).Failed())
  {
    ezLog::Error("Failed to delete the previous test project '{}'.", out_sPath);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezStatus ezEditorTestProjectCreation::CreateBlankProject()
{
  ezStringBuilder sProjectPath;
  if (PrepareTargetFolder(sProjectPath, "BlankProject").Failed())
    return ezStatus("Failed to prepare the target folder");

  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-createProject");
  arguments.PushBack(sProjectPath);
  arguments.PushBack("-pluginTemplate");
  arguments.PushBack("General3D");

  EZ_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  // the plugin selection is the only thing the creation writes for a blank project, the 'ezProject'
  // file is written when the new project is opened afterwards - both have to be there
  ezStringBuilder sFile(sProjectPath, "/Editor/PluginSelection.ddl");
  EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  sFile.Set(sProjectPath, "/ezProject");
  EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  return EZ_SUCCESS;
}

ezStatus ezEditorTestProjectCreation::CreateProjectFromTemplate(ezStringView sTemplate)
{
  ezStringBuilder sProjectPath;
  if (PrepareTargetFolder(sProjectPath, "TemplateProject").Failed())
    return ezStatus("Failed to prepare the target folder");

  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-createProject");
  arguments.PushBack(sProjectPath);
  arguments.PushBack("-projectTemplate");
  arguments.PushBack(sTemplate);

  EZ_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  ezStringBuilder sFile(sProjectPath, "/ezProject");
  EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sFile), "'%s' was not created", sFile.GetData());

  // the template brings its own plugin selection, and content that a blank project does not have
  sFile.Set(sProjectPath, "/Editor/PluginSelection.ddl");
  EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sFile), "'%s' was not copied from the template", sFile.GetData());

  sFile.Set(sProjectPath, "/Scenes");
  EZ_TEST_BOOL_MSG(ezOSFile::ExistsDirectory(sFile), "'%s' was not copied from the template", sFile.GetData());

  m_sTemplateProjectPath = sProjectPath;
  return EZ_SUCCESS;
}

ezStatus ezEditorTestProjectCreation::TransformTemplateProject(ezStringView sTemplate)
{
  // so that this sub-test can also run on its own, without the one that creates the project
  if (m_sTemplateProjectPath.IsEmpty())
  {
    EZ_SUCCEED_OR_RETURN(CreateProjectFromTemplate(sTemplate));
  }

  ezDynamicArray<ezString> arguments;
  arguments.PushBack("-project");
  arguments.PushBack(m_sTemplateProjectPath);
  arguments.PushBack("-transform");
  arguments.PushBack("Default");
  arguments.PushBack("-outputDir");

  ezStringBuilder sUserDataDir = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sUserDataDir.AppendPath(GetTestName());
  arguments.PushBack(sUserDataDir);

  EZ_SUCCEED_OR_RETURN(RunEditorProcessor(arguments));

  return ezStatus(EZ_SUCCESS);
}
