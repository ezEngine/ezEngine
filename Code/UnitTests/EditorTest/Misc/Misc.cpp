#include <EditorTestPCH.h>

#include "Misc.h"
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <EditorFramework/Assets/AssetCurator.h>

static ezEditorTestMisc s_GameEngineTestBasics;

const char* ezEditorTestMisc::GetTestName() const
{
  return "Misc Tests";
}

void ezEditorTestMisc::SetupSubTests()
{
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
}

ezResult ezEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorTestMisc::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorTestMisc::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::GameObjectReferences)
  {
    m_pDocument = SUPER::OpenDocument("Scenes/GameObjectReferences.ezScene");

    ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(m_pDocument);

    pWindow->CreateImageCapture("D:/test.tga");

    if (EZ_TEST_BOOL(m_pDocument!= nullptr).Failed())
      return ezTestAppRun::Quit;

    for (int i = 0; i < 10; ++i)
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(100));
      ProcessEvents();
    }

    pWindow->CreateImageCapture("D:/test2.tga");

    for (int i = 0; i < 10; ++i)
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(100));
      ProcessEvents();
    }
    
  }


  //const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();
  //for (auto* pDesc : allDesc)
  //{
  //  if (pDesc->m_bCanCreate)
  //  {
  //    ezStringBuilder sName = m_sProjectPath;
  //    sName.AppendPath(pDesc->m_sDocumentTypeName);
  //    sName.ChangeFileExtension(pDesc->m_sFileExtension);
  //    ezDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow);
  //    EZ_TEST_BOOL(pDoc);
  //    ProcessEvents();
  //  }
  //}
  //// Make sure the engine process did not crash after creating every kind of document.
  //EZ_TEST_BOOL(!ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  ////TODO: Newly created assets actually do not transform cleanly.
  //if (false)
  //{
  //  ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::TriggeredManually);

  //  ezUInt32 uiNumAssets;
  //  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
  //  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::TransformError], 0);
  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingDependency], 0);
  //  EZ_TEST_INT(sections[ezAssetInfo::TransformState::MissingReference], 0);
  //}
  return ezTestAppRun::Quit;
}

ezResult ezEditorTestMisc::InitializeSubTest(ezInt32 iIdentifier)
{
  return EZ_SUCCESS;
}

ezResult ezEditorTestMisc::DeInitializeSubTest(ezInt32 iIdentifier)
{
  ezDocumentManager::CloseAllDocuments();
  return EZ_SUCCESS;
}

