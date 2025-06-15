#include <EditorTest/EditorTestPCH.h>

#include "Project.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <TestFramework/Framework/TestFramework.h>

static ezEditorTestProject s_EditorTestProject;

const char* ezEditorTestProject::GetTestName() const
{
  return "Project Tests";
}

void ezEditorTestProject::SetupSubTests()
{
  AddSubTest("Create Documents", SubTests::ST_CreateDocuments);
  AddSubTest("Create C++ Solution", SubTests::ST_CreateCppSolution);
}

ezResult ezEditorTestProject::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::CreateAndLoadProject("TestProject").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorTestProject::DeInitializeTest()
{
  if (!m_sProjectPath.IsEmpty())
  {
    // The build results take up vast amounts of memory and prolong test result upload.
    ezStringBuilder buildOutput = m_sProjectPath;
    buildOutput.AppendPath("CppSource", "Build");
    ezOSFile::DeleteFolder(buildOutput).IgnoreResult();
  }

  // For profiling the doc creation.
  // SafeProfilingData();
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestProject::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case ST_CreateDocuments:
      return CreateDocuments();
    case ST_CreateCppSolution:
      return CreateCppSolution();
    default:
      EZ_REPORT_FAILURE("missing case statement");
  }

  return ezTestAppRun::Quit;
}

ezTestAppRun ezEditorTestProject::CreateDocuments()
{
  const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();
  for (auto it : allDesc)
  {
    auto pDesc = it.Value();

    if (pDesc->m_bCanCreate)
    {
      ezStringBuilder sName = m_sProjectPath;
      sName.AppendPath(pDesc->m_sDocumentTypeName);
      sName.ChangeFileExtension(pDesc->m_sFileExtension);
      ezDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow);
      EZ_TEST_BOOL(pDoc);
      ProcessEvents();
    }
  }
  // Make sure the engine process did not crash after creating every kind of document.
  EZ_TEST_BOOL(!ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  // TODO: Newly created assets actually do not transform cleanly.
  if (false)
  {
    ezAssetCurator::GetSingleton()->TransformAllAssets().IgnoreResult();

    ezUInt32 uiNumAssets;
    ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
    ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

    EZ_TEST_INT(sections[ezAssetInfo::TransformState::TransformError], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingTransformDependency], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingPackageDependency], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingThumbnailDependency], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::CircularDependency], 0);
  }
  return ezTestAppRun::Quit;
}

ezTestAppRun ezEditorTestProject::CreateCppSolution()
{
  ezCppSettings cpp;
  EZ_TEST_BOOL(cpp.Load().Failed());

  EZ_TEST_BOOL(!ezCppProject::ExistsSolution(cpp));

  cpp.m_sPluginName = "TestPlugin";

  EZ_TEST_RESULT(cpp.Save());
  EZ_TEST_RESULT(ezCppProject::CleanBuildDir(cpp));
  EZ_TEST_RESULT(ezCppProject::PopulateWithDefaultSources(cpp));
  if (!EZ_TEST_RESULT(ezCppProject::RunCMake(cpp)))
    return ezTestAppRun::Quit;

  EZ_TEST_BOOL(ezCppProject::ExistsProjectCMakeListsTxt());
  EZ_TEST_BOOL(ezCppProject::ExistsSolution(cpp));
  EZ_TEST_RESULT(ezCppProject::BuildCodeIfNecessary(cpp));

  ezCppProject::UpdatePluginConfig(cpp);
  ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  ProcessEvents(20);

  EZ_TEST_BOOL(!ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
  return ezTestAppRun::Quit;
}
