#include <EditorTestPCH.h>

#include "Project.h"
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <EditorFramework/Assets/AssetCurator.h>

static ezEditorTestProject s_GameEngineTestBasics;

const char* ezEditorTestProject::GetTestName() const
{
  return "Project Tests";
}

void ezEditorTestProject::SetupSubTests()
{
  AddSubTest("Create Documents", SubTests::ST_CreateDocuments);
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
  // For profiling the doc creation.
  //SafeProfilingData();
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestProject::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();
  for (auto* pDesc : allDesc)
  {
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

  //TODO: Newly created assets actually do not transform cleanly.
  if (false)
  {
    ezAssetCurator::GetSingleton()->TransformAllAssets();

    ezUInt32 uiNumAssets;
    ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
    ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

    EZ_TEST_INT(sections[ezAssetInfo::TransformState::TransformError], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingDependency], 0);
    EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingReference], 0);
  }
  return ezTestAppRun::Quit;
}

