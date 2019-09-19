#include <EditorTestPCH.h>

#include "Misc.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>

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

    if (EZ_TEST_BOOL(m_pDocument != nullptr).Failed())
      return ezTestAppRun::Quit;

    ezAssetCurator::GetSingleton()->TransformAsset(m_pDocument->GetGuid(), ezTransformFlags::Default);

    ezQtEngineDocumentWindow* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(ezQtDocumentWindow::FindWindowByDocument(m_pDocument));

    if (EZ_TEST_BOOL(pWindow != nullptr).Failed())
      return ezTestAppRun::Quit;

    auto viewWidgets = pWindow->GetViewWidgets();

    if (EZ_TEST_BOOL(!viewWidgets.IsEmpty()).Failed())
      return ezTestAppRun::Quit;

    ezQtEngineViewWidget::InteractionContext ctxt;
    ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
    ezQtEngineViewWidget::SetInteractionContext(ctxt);

    viewWidgets[0]->m_pViewConfig->m_RenderMode = ezViewRenderMode::Default;
    viewWidgets[0]->m_pViewConfig->m_Perspective = ezSceneViewPerspective::Perspective;
    viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

    ExecuteDocumentAction("Scene.Camera.JumpTo.0", m_pDocument, true);

    for (int i = 0; i < 10; ++i)
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(100));
      ProcessEvents();
    }

    EZ_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

    EZ_TEST_IMAGE(1, 100);
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
