#include <EditorTest/EditorTestPCH.h>

#include "TestClass.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QMimeData>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezEditorTestApplication::ezEditorTestApplication()
  : ezApplication("ezEditor")
{
  EnableMemoryLeakReporting(true);

  m_pEditorApp = new ezQtEditorApp;
}

ezResult ezEditorTestApplication::BeforeCoreSystemsStartup()
{
  if (SUPER::BeforeCoreSystemsStartup().Failed())
    return EZ_FAILURE;

  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("editor");
  ezStartup::AddApplicationTag("editorapp");

  ezQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  return EZ_SUCCESS;
}

void ezEditorTestApplication::AfterCoreSystemsShutdown()
{
  ezQtEditorApp::GetSingleton()->DeInitQt();

  delete m_pEditorApp;
  m_pEditorApp = nullptr;
}

void ezEditorTestApplication::Run()
{
  qApp->processEvents();
}

void ezEditorTestApplication::AfterCoreSystemsStartup()
{
  EZ_PROFILE_SCOPE("AfterCoreSystemsStartup");
  // We override the user data dir to not pollute the editor settings.
  ezStringBuilder userDataDir = ezOSFile::GetUserDataFolder();
  userDataDir.AppendPath("ezEngine Project", "EditorTest");
  userDataDir.MakeCleanPath();

  ezQtEditorApp::GetSingleton()->StartupEditor(ezQtEditorApp::StartupFlags::SafeMode | ezQtEditorApp::StartupFlags::NoRecent | ezQtEditorApp::StartupFlags::UnitTest, userDataDir);
  // Disable msg boxes.
  ezQtUiServices::SetHeadless(true);
  ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

  ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezDataDirUsage::AllowWrites).IgnoreResult();
}

void ezEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  EZ_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  ezQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

ezEditorTest::ezEditorTest()
{
  ezQtEngineViewWidget::s_FixedResolution = ezSizeU32(512, 512);
}

ezEditorTest::~ezEditorTest() = default;

ezEditorTestApplication* ezEditorTest::CreateApplication()
{
  ezEditorTestApplication* pTestApplication = EZ_DEFAULT_NEW(ezEditorTestApplication);

  m_CommandLineArguments = ezCommandLineUtils::GetGlobalInstance()->GetCommandLineArray();
  EZ_ASSERT_DEV(m_CommandLineArguments.GetCount() > 0, "There should always be at least 1 command line argument (the executable name)");

  m_CommandLineArgumentPointers.Clear();
  for (auto& s : m_CommandLineArguments)
  {
    m_CommandLineArgumentPointers.PushBack(s.GetData());
  }

  pTestApplication->SetCommandLineArguments(m_CommandLineArgumentPointers.GetCount(), m_CommandLineArgumentPointers.GetData());

  return pTestApplication;
}

ezResult ezEditorTest::GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber)
{
  if (!m_CapturedImage.IsValid())
    return EZ_FAILURE;

  ref_img.ResetAndMove(std::move(m_CapturedImage));
  return EZ_SUCCESS;
}

ezResult ezEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(ezRun_Startup(m_pApplication));

  static bool s_bCheckedReferenceDriver = false;
  static bool s_bIsReferenceDriver = false;
  static bool s_bIsAMDDriver = false;

  if (!s_bCheckedReferenceDriver)
  {
    s_bCheckedReferenceDriver = true;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezUniquePtr<ezGALDevice> pDevice;
    ezGALDeviceCreationDescription DeviceInit;

    pDevice = ezGALDeviceFactory::CreateDevice(ezGameApplication::GetActiveRenderer(), ezFoundation::GetDefaultAllocator(), DeviceInit);

    EZ_SUCCEED_OR_RETURN(pDevice->Init());

    if (pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      s_bIsReferenceDriver = true;
    }
    else if (pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      s_bIsAMDDriver = true;
    }

    EZ_SUCCEED_OR_RETURN(pDevice->Shutdown());
    pDevice.Clear();
#endif
  }

  if (ezGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && s_bIsReferenceDriver)
  {
    // Use different images for comparison when running the D3D11 Reference Device
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
  }
  else if (ezGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && s_bIsAMDDriver)
  {
    // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
  }
  else
  {
    ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
  }

  return EZ_SUCCESS;
}

ezResult ezEditorTest::DeInitializeTest()
{
  CloseCurrentProject();

  if (m_pApplication)
  {
    ezRun_Shutdown(m_pApplication);

    EZ_DEFAULT_DELETE(m_pApplication);
  }


  return EZ_SUCCESS;
}

ezResult ezEditorTest::CreateAndLoadProject(const char* name)
{
  EZ_PROFILE_SCOPE("CreateAndLoadProject");
  ezStringBuilder relPath;
  relPath = ":APPDATA";
  relPath.AppendPath(name);

  ezStringBuilder absPath;
  if (ezFileSystem::ResolvePath(relPath, &absPath, nullptr).Failed())
  {
    ezLog::Error("Failed to resolve project path '{0}'.", relPath);
    return EZ_FAILURE;
  }
  if (ezOSFile::DeleteFolder(absPath).Failed())
  {
    ezLog::Error("Failed to delete old project folder '{0}'.", absPath);
    return EZ_FAILURE;
  }

  ezStringBuilder projectFile = absPath;
  projectFile.AppendPath("ezProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(true, projectFile).Failed())
  {
    ezLog::Error("Failed to create project '{0}'.", projectFile);
    return EZ_FAILURE;
  }

  m_sProjectPath = absPath;
  return EZ_SUCCESS;
}

ezResult ezEditorTest::OpenProject(const char* path)
{
  EZ_PROFILE_SCOPE("OpenProject");
  ezStringBuilder relPath;
  relPath = ">sdk";
  relPath.AppendPath(path);

  ezStringBuilder absPath;
  if (ezFileSystem::ResolveSpecialDirectory(relPath, absPath).Failed())
  {
    ezLog::Error("Failed to resolve project path '{0}'.", relPath);
    return EZ_FAILURE;
  }

  // Copy project to temp folder
  ezStringBuilder projectName = ezPathUtils::GetFileName(path);
  ezStringBuilder relTempPath;
  relTempPath = ":APPDATA";
  relTempPath.AppendPath(projectName);

  ezStringBuilder absTempPath;
  if (ezFileSystem::ResolvePath(relTempPath, &absTempPath, nullptr).Failed())
  {
    ezLog::Error("Failed to resolve project temp path '{0}'.", relPath);
    return EZ_FAILURE;
  }
  if (ezOSFile::DeleteFolder(absTempPath).Failed())
  {
    ezLog::Error("Failed to delete old project temp folder '{0}'.", absTempPath);
    return EZ_FAILURE;
  }
  if (ezOSFile::CopyFolder(absPath, absTempPath).Failed())
  {
    ezLog::Error("Failed to copy project '{0}' to temp location: '{1}'.", absPath, absTempPath);
    return EZ_FAILURE;
  }


  ezStringBuilder projectFile = absTempPath;
  projectFile.AppendPath("ezProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(false, projectFile).Failed())
  {
    ezLog::Error("Failed to open project '{0}'.", projectFile);
    return EZ_FAILURE;
  }

  m_sProjectPath = absTempPath;
  return EZ_SUCCESS;
}

ezDocument* ezEditorTest::OpenDocument(const char* subpath)
{
  ezStringBuilder fullpath;
  fullpath = m_sProjectPath;
  fullpath.AppendPath(subpath);

  ezDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(fullpath, ezDocumentFlags::RequestWindow);

  if (pDoc)
  {
    ProcessEvents();
  }

  return pDoc;
}

void ezEditorTest::ExecuteDocumentAction(const char* szActionName, ezDocument* pDocument, const ezVariant& argument /*= ezVariant()*/)
{
  EZ_TEST_BOOL(ezActionManager::ExecuteAction(nullptr, szActionName, pDocument, argument).Succeeded());
}

ezResult ezEditorTest::CaptureImage(ezQtDocumentWindow* pWindow, const char* szImageName)
{
  ezStringBuilder sImgPath = ezOSFile::GetUserDataFolder("EditorTests");
  sImgPath.AppendFormat("/{}.tga", szImageName);

  ezOSFile::DeleteFile(sImgPath).IgnoreResult();

  pWindow->CreateImageCapture(sImgPath);

  for (int i = 0; i < 10; ++i)
  {
    ProcessEvents();

    if (ezOSFile::ExistsFile(sImgPath))
      break;

    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
  }

  if (!ezOSFile::ExistsFile(sImgPath))
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(m_CapturedImage.LoadFrom(sImgPath));

  return EZ_SUCCESS;
}

void ezEditorTest::CloseCurrentProject()
{
  EZ_PROFILE_SCOPE("CloseCurrentProject");
  m_sProjectPath.Clear();
  m_pApplication->m_pEditorApp->CloseProject();
}

void ezEditorTest::SafeProfilingData()
{
  ezProfilingUtils::SaveProfilingCapture(":appdata/profiling.json").IgnoreResult();
}

void ezEditorTest::ProcessEvents(ezUInt32 uiIterations)
{
  EZ_PROFILE_SCOPE("ProcessEvents");
  if (qApp)
  {
    for (ezUInt32 i = 0; i < uiIterations; i++)
    {
      qApp->processEvents();
    }
  }
}

std::unique_ptr<QMimeData> ezEditorTest::AssetsToDragMimeData(ezArrayPtr<ezUuid> assetGuids)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  ezStringBuilder tmp;

  stream << (int)1;
  for (ezUInt32 i = 0; i < assetGuids.GetCount(); ++i)
  {
    QString sGuid(ezConversionUtils::ToString(assetGuids[i], tmp).GetData());
    stream << sGuid;
  }

  mimeData->setData("application/ezEditor.AssetGuid", encodedData);
  return std::move(mimeData);
}

std::unique_ptr<QMimeData> ezEditorTest::ObjectsDragMimeData(const ezDeque<const ezDocumentObject*>& objects)
{
  ezHybridArray<const ezDocumentObject*, 32> Dragged;
  for (const ezDocumentObject* pObject : objects)
  {
    Dragged.PushBack(pObject);
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << Dragged;

  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  mimeData->setData("application/ezEditor.ObjectSelection", encodedData);
  return std::move(mimeData);
}

void ezEditorTest::MoveObjectsToLayer(ezScene2Document* pDoc, const ezDeque<const ezDocumentObject*>& objects, const ezUuid& layer, ezDeque<const ezDocumentObject*>& new_objects)
{
  pDoc->GetSelectionManager()->SetSelection(objects);

  ezQtLayerAdapter adapter(pDoc);
  auto mimeData = ObjectsDragMimeData(objects);
  ezDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = -1;
  info.m_pMimeData = mimeData.get();
  info.m_sTargetContext = "layertree";
  info.m_TargetDocument = pDoc->GetGuid();
  info.m_TargetObject = pDoc->GetLayerObject(layer)->GetGuid();
  info.m_bCtrlKeyDown = false;
  info.m_bShiftKeyDown = false;
  info.m_pAdapter = &adapter;
  if (!EZ_TEST_BOOL(ezDragDropHandler::DropOnly(&info)))
    return;

  new_objects = pDoc->GetLayerDocument(layer)->GetSelectionManager()->GetSelection();
}

const ezDocumentObject* ezEditorTest::DropAsset(ezScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift /*= false*/, bool bCtrl /*= false*/)
{
  const ezAssetCurator::ezLockedSubAsset asset = ezAssetCurator::GetSingleton()->FindSubAsset(szAssetGuidOrPath);
  if (EZ_TEST_BOOL(asset.isValid()))
  {
    ezUuid assetGuid = asset->m_Data.m_Guid;
    ezArrayPtr<ezUuid> assets(&assetGuid, 1);
    auto mimeData = AssetsToDragMimeData(assets);

    ezDragDropInfo info;
    info.m_pMimeData = mimeData.get();
    info.m_TargetDocument = pDoc->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_iTargetObjectSubID = 0;
    info.m_bShiftKeyDown = bShift;
    info.m_bCtrlKeyDown = bCtrl;

    if (EZ_TEST_BOOL(ezDragDropHandler::DropOnly(&info)))
    {
      return pDoc->GetSelectionManager()->GetCurrentObject();
    }
  }
  return {};
}

const ezDocumentObject* ezEditorTest::CreateGameObject(ezScene2Document* pDoc, const ezDocumentObject* pParent, ezStringView sName)
{
  auto pAccessor = pDoc->GetObjectAccessor();
  pAccessor->StartTransaction("Add Game Object");

  ezUuid guid;
  EZ_TEST_STATUS(pAccessor->AddObjectByName(pParent != nullptr ? pParent : pDoc->GetObjectManager()->GetRootObject(), "Children", -1, ezRTTI::FindTypeByName("ezGameObject"), guid));
  const ezDocumentObject* pObject = pAccessor->GetObject(guid);
  EZ_TEST_STATUS(pAccessor->SetValueByName(pObject, "Name", sName));

  pAccessor->FinishTransaction();

  return pObject;
}
