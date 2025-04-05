#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/MaterialDocument/MaterialDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
// #include <EditorPlugins/Assets/EditorPluginAssets/MaterialAsset/MaterialAsset.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <QBackingStore>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
static ezMaterialDocumentTest s_EditorSceneDocumentTest;

const char* ezMaterialDocumentTest::GetTestName() const
{
  return "Material Document Tests";
}

void ezMaterialDocumentTest::SetupSubTests()
{
  AddSubTest("Create New Material FromShader", SubTests::ST_CreateNewMaterialFromShader);
  AddSubTest("Create New Material FromBase", SubTests::ST_CreateNewMaterialFromBase);
  AddSubTest("Create New Material FromVSE", SubTests::ST_CreateNewMaterialFromVSE);
  AddSubTest("Create New Material FromTemplate", SubTests::ST_CreateNewMaterialFromTemplate);
}

ezResult ezMaterialDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return EZ_FAILURE;

  if (ezStatus res = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None); res.Failed())
  {
    ezLog::Error("Asset transform failed: {}", res.m_sMessage);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezMaterialDocumentTest::DeInitializeTest()
{
  m_pDoc = nullptr;
  m_MaterialGuid = ezUuid::MakeInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezMaterialDocumentTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_CreateNewMaterialFromShader:
      CreateMaterialFromShader();
      break;
    case SubTests::ST_CreateNewMaterialFromBase:
      CreateMaterialFromBase();
      break;
    case SubTests::ST_CreateNewMaterialFromVSE:
      CreateMaterialFromVSE();
      break;
    case SubTests::ST_CreateNewMaterialFromTemplate:
      CreateMaterialFromTemplate();
      break;
  }
  return ezTestAppRun::Quit;
}

ezResult ezMaterialDocumentTest::CreateMaterial(const char* szSceneName)
{
  ezStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(m_pDoc != nullptr))
      return EZ_FAILURE;

    EZ_ANALYSIS_ASSUME(m_pDoc != nullptr);
    m_MaterialGuid = m_pDoc->GetGuid();
    ProcessEvents();
  }
  return EZ_SUCCESS;
}

void ezMaterialDocumentTest::CloseMaterial()
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Close Document")
  {
    bool bSaved = false;
    ezTaskGroupID id = m_pDoc->SaveDocumentAsync(
      [&bSaved](ezDocument* pDoc, ezStatus res)
      {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(bSaved);
    m_pDoc = nullptr;
    m_MaterialGuid = ezUuid::MakeInvalid();
  }
}

const ezDocumentObject* ezMaterialDocumentTest::GetShaderProperties(const ezDocumentObject* pMaterialProperties)
{
  auto pAccessor = m_pDoc->GetObjectAccessor();
  ezVariant vChildGuild;
  EZ_TEST_STATUS(pAccessor->GetValueByName(pMaterialProperties, "ShaderProperties", vChildGuild));
  EZ_TEST_BOOL(vChildGuild.IsValid() && vChildGuild.CanConvertTo<ezUuid>());
  ezUuid childGuild = vChildGuild.Get<ezUuid>();
  return pAccessor->GetObject(childGuild);
}

void ezMaterialDocumentTest::CreateMaterialFromShader()
{
  if (CreateMaterial("CreateMaterialFromShader.ezMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();
  const ezDocumentObject* pProperties = m_pDoc->GetSelectionManager()->GetCurrentObject();

  pAccessor->StartTransaction("Change Property 'Shader Mode'");
  EZ_TEST_STATUS(pAccessor->SetValueByName(pProperties, "ShaderMode", 1));
  pAccessor->FinishTransaction();

  ProcessEvents();

  pAccessor->StartTransaction("Change Property 'Shader'");
  EZ_TEST_STATUS(pAccessor->SetValueByName(pProperties, "Shader", "Shaders/Materials/DefaultMaterial.ezShader"));
  pAccessor->FinishTransaction();

  ProcessEvents();

  const ezDocumentObject* pShaderProperties = GetShaderProperties(pProperties);
  pAccessor->StartTransaction("Change Property 'SHADING MODE'");
  EZ_TEST_STATUS(pAccessor->SetValueByName(pShaderProperties, "SHADING_MODE", 1));
  pAccessor->FinishTransaction();

  // m_pDoc->GetCommandHistory()->ClearUndoHistory();

  ezQtEngineDocumentWindow* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(ezQtDocumentWindow::FindWindowByDocument(m_pDoc));
  if (!EZ_TEST_BOOL(pWindow != nullptr))
    return;

  pWindow->backingStore()->window()->showMaximized();

  EZ_ANALYSIS_ASSUME(pWindow != nullptr);
  auto viewWidgets = pWindow->GetViewWidgets();

  if (!EZ_TEST_BOOL(!viewWidgets.IsEmpty()))
    return;

  ezQtEngineViewWidget::InteractionContext ctxt;
  ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
  ezQtEngineViewWidget::SetInteractionContext(ctxt);

  viewWidgets[0]->m_pViewConfig->m_RenderMode = ezViewRenderMode::Default;
  viewWidgets[0]->m_pViewConfig->m_Perspective = ezSceneViewPerspective::Perspective;
  viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

  ezActionContext ctx2;
  ctx2.m_pDocument = m_pDoc;
  ctx2.m_pWindow = viewWidgets[0];

  ezActionManager::ExecuteAction(nullptr, "View.SkyBox", ctx2, false).AssertSuccess();
  ProcessEvents();

  for (int i = 0; i < 10; ++i)
  {
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    ProcessEvents();
  }

  EZ_TEST_BOOL(CaptureImage(pWindow, "MatFromShader").Succeeded());
  EZ_TEST_IMAGE(1, 100);

  // ProcessEvents(999999999);
  CloseMaterial();
}


void ezMaterialDocumentTest::CreateMaterialFromBase()
{
  if (CreateMaterial("CreateMaterialFromBase.ezMaterialAsset").Failed())
    return;

  ProcessEvents(10);
  CloseMaterial();
}

void ezMaterialDocumentTest::CreateMaterialFromVSE()
{
  if (CreateMaterial("CreateMaterialFromVSE.ezMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  ProcessEvents(9999999);
  CloseMaterial();
}

void ezMaterialDocumentTest::CreateMaterialFromTemplate()
{
  if (CreateMaterial("CreateMaterialFromTemplate.ezMaterialAsset").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  ProcessEvents(10);
  CloseMaterial();
}
