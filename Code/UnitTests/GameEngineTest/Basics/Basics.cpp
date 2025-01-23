#include <GameEngineTest/GameEngineTestPCH.h>

#include "Basics.h"
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <GameEngineTest/SubstanceTest/SubstanceTest.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
ezResult TranformProject(const char* szProjectPath, ezUInt32 uiCleanVersion)
{
  ezGlobalLog::AddLogWriter(&ezLogWriter::Console::LogMessageHandler);
  EZ_SCOPE_EXIT(ezGlobalLog::RemoveLogWriter(&ezLogWriter::Console::LogMessageHandler));

  ezStringBuilder sBinPath = ezOSFile::GetApplicationDirectory();

  ezStringBuilder sProjectDir;
  if (ezPathUtils::IsAbsolutePath(szProjectPath))
  {
    sProjectDir = szProjectPath;
    sProjectDir.MakeCleanPath();
  }
  else
  {
    // Assume to be relative to ez root.
    sProjectDir = ezFileSystem::GetSdkRootDirectory();
    sProjectDir.AppendPath(szProjectPath);
    sProjectDir.MakeCleanPath();
  }

  ezLog::Info("Transforming assets for project '{}'", sProjectDir);

  {
    ezStringBuilder sProjectAssetDir = sProjectDir;
    sProjectAssetDir.PathParentDirectory();
    sProjectAssetDir.AppendPath("AssetCache");

    ezStringBuilder sCleanFile = sProjectAssetDir;
    sCleanFile.AppendPath("CleanVersion.dat");

    ezUInt32 uiTargetVersion = 0;
    ezOSFile f;

    if (f.Open(sCleanFile, ezFileOpenMode::Read, ezFileShareMode::Default).Succeeded())
    {
      f.Read(&uiTargetVersion, sizeof(ezUInt32));
      f.Close();

      ezLog::Info("CleanVersion.dat exists -> project has been transformed before.");
    }

    if (uiTargetVersion != uiCleanVersion)
    {
      ezLog::Info("Clean version {} != {} -> deleting asset cache.", uiTargetVersion, uiCleanVersion);

#  if (EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS) && EZ_ENABLED(EZ_SUPPORTS_FILE_STATS))
      if (ezOSFile::DeleteFolder(sProjectAssetDir).Failed())
      {
        ezLog::Warning("Deleting the asset cache folder failed.");
      }
#  endif

      if (f.Open(sCleanFile, ezFileOpenMode::Write, ezFileShareMode::Default).Succeeded())
      {
        f.Write(&uiCleanVersion, sizeof(ezUInt32)).IgnoreResult();
        f.Close();
      }
    }
    else
    {
      ezLog::Info("Clean version {} == {}.", uiTargetVersion, uiCleanVersion);
    }
  }

  sBinPath.AppendPath("ezEditorProcessor.exe");
  sBinPath.MakeCleanPath();

  ezStringBuilder sOutputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  {
    ezStringView sProjectPath = ezPathUtils::GetFileDirectory(szProjectPath);
    sProjectPath.Trim("\\/");
    ezStringView sProjectName = ezPathUtils::GetFileName(sProjectPath);
    sOutputPath.AppendPath("Transform");
    sOutputPath.Append(sProjectName);
    if (ezOSFile::CreateDirectoryStructure(sOutputPath).Failed())
      ezLog::Error("Failed to create output directory: {}", sOutputPath);
  }

  ezStringBuilder sStdout;
  ezMutex mutex;

  ezProcessOptions opt;
  opt.m_onStdOut = [&sStdout, &mutex](ezStringView sView)
  {
    EZ_LOCK(mutex);
    sStdout.Append(sView);
  };
  opt.m_onStdError = [&sStdout, &mutex](ezStringView sView)
  {
    EZ_LOCK(mutex);
    sStdout.Append(sView);
  };

  opt.m_sProcess = sBinPath;
  opt.m_Arguments.PushBack("-project");
  opt.AddArgument("\"{0}\"", sProjectDir);
  opt.m_Arguments.PushBack("-transform");
  opt.m_Arguments.PushBack("Default");
  opt.m_Arguments.PushBack("-outputDir");
  opt.AddArgument("\"{0}\"", sOutputPath);
  opt.m_Arguments.PushBack("-noRecent");
  opt.m_Arguments.PushBack("-AssetThumbnails");
  opt.m_Arguments.PushBack("never");
  opt.m_Arguments.PushBack("-renderer");
  opt.m_Arguments.PushBack(ezGameApplication::GetActiveRenderer());
  opt.m_Arguments.PushBack("-fullcrashdumps");

  ezProcess proc;
  ezLog::Info("Launching: '{0}'", sBinPath);
  ezResult res = proc.Launch(opt);
  if (res.Failed())
  {
    proc.Terminate().IgnoreResult();
    ezLog::Error("Failed to start process: '{0}'", sBinPath);
  }

  ezTime timeout = ezTime::MakeFromMinutes(15);
  res = proc.WaitToFinish(timeout);
  ezResult returnValue = EZ_SUCCESS;
  if (res.Failed())
  {
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    ezStringBuilder sDumpFile = sOutputPath;
    sDumpFile.AppendPath("Timeout.dmp");
    ezMiniDumpUtils::WriteExternalProcessMiniDump(sDumpFile, proc.GetProcessID()).LogFailure();
#  endif
    proc.Terminate().IgnoreResult();
    ezLog::Error("Process timeout ({1}): '{0}'", sBinPath, timeout);
    returnValue = EZ_FAILURE;
  }
  else if (proc.GetExitCode() != 0)
  {
    ezLog::Error("Process failure ({0}): ExitCode: '{1}'", sBinPath, proc.GetExitCode());
    returnValue = EZ_FAILURE;
  }
  else
  {
    ezLog::Success("Executed Asset Processor to transform '{}'", szProjectPath);
  }

  {
    ezOSFile fileWriter;
    ezStringBuilder sLogFile = sOutputPath;
    sLogFile.AppendFormat("/EditorProcessor_{}.txt", proc.GetProcessID());
    if (fileWriter.Open(sLogFile, ezFileOpenMode::Write, ezFileShareMode::Exclusive).Failed() || fileWriter.Write(sStdout.GetData(), sStdout.GetElementCount()).Failed())
    {
      ezLog::Error("Failed to write EditorProcessor log at '{}'", sLogFile);
    }
  }

  return returnValue;
}
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
EZ_CREATE_SIMPLE_TEST_GROUP(00_Init);

EZ_CREATE_SIMPLE_TEST(00_Init, 00_TransformBase) // prefix with 00_ to ensure base data is transformed first
{
  EZ_TEST_BOOL(TranformProject("Data/Base/ezProject", 2).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformBasics)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Basics/ezProject", 2).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformParticles)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Particles/ezProject", 4).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformTypeScript)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/TypeScript/ezProject", 5).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformEffects)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Effects/ezProject", 4).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformAnimations)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Animations/ezProject", 7).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformStateMachine)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/StateMachine/ezProject", 7).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformPlatformWin)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/PlatformWin/ezProject", 5).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformXR)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/XR/ezProject", 5).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformVisualScript)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/VisualScript/ezProject", 6).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformSubstance)
{
  if (ezGameEngineTestSubstance::HasSubstanceDesignerInstalled())
  {
    EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Substance/ezProject", 1).Succeeded());
  }
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformProcGen)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/ProcGen/ezProject", 6).Succeeded());
}

EZ_CREATE_SIMPLE_TEST(00_Init, TransformRmlUi)
{
  EZ_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/RmlUi/ezProject", 1).Succeeded());
}

#endif


static ezGameEngineTestBasics s_GameEngineTestBasics;

const char* ezGameEngineTestBasics::GetTestName() const
{
  return "Basic Engine Tests";
}

ezGameEngineTestApplication* ezGameEngineTestBasics::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication_Basics);
  return m_pOwnApplication;
}

void ezGameEngineTestBasics::SetupSubTests()
{
  AddSubTest("Many Meshes", SubTests::ManyMeshes);
  AddSubTest("Skybox", SubTests::Skybox);
  AddSubTest("Debug Rendering", SubTests::DebugRendering);
  AddSubTest("Debug Rendering - No Lines", SubTests::DebugRendering2);
  AddSubTest("Load Scene", SubTests::LoadScene);
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
}

ezResult ezGameEngineTestBasics::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  if (iIdentifier == SubTests::ManyMeshes)
  {
    m_pOwnApplication->SubTestManyMeshesSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::Skybox)
  {
    m_pOwnApplication->SubTestSkyboxSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::DebugRendering || iIdentifier == SubTests::DebugRendering2)
  {
    m_pOwnApplication->SubTestDebugRenderingSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::LoadScene)
  {
    m_pOwnApplication->SubTestLoadSceneSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::GameObjectReferences)
  {
    m_pOwnApplication->SubTestGoReferenceSetup();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestBasics::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ManyMeshes)
    return m_pOwnApplication->SubTestManyMeshesExec(m_iFrame);

  if (iIdentifier == SubTests::Skybox)
    return m_pOwnApplication->SubTestSkyboxExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering)
    return m_pOwnApplication->SubTestDebugRenderingExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering2)
    return m_pOwnApplication->SubTestDebugRenderingExec2(m_iFrame);

  if (iIdentifier == SubTests::LoadScene)
    return m_pOwnApplication->SubTestLoadSceneExec(m_iFrame);

  if (iIdentifier == SubTests::GameObjectReferences)
    return m_pOwnApplication->SubTestGoReferenceExec(m_iFrame);

  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

ezGameEngineTestApplication_Basics::ezGameEngineTestApplication_Basics()
  : ezGameEngineTestApplication("Basics")
{
}

void ezGameEngineTestApplication_Basics::SubTestManyMeshesSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezBinMesh");

  ezInt32 dim = 15;

  for (ezInt32 z = -dim; z <= dim; ++z)
  {
    for (ezInt32 y = -dim; y <= dim; ++y)
    {
      for (ezInt32 x = -dim; x <= dim; ++x)
      {
        ezGameObjectDesc go;
        go.m_LocalPosition.Set(x * 5.0f, y * 5.0f, z * 5.0f);

        ezGameObject* pObject;
        m_pWorld->CreateObject(go, pObject);

        ezMeshComponent* pMesh;
        m_pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);

        pMesh->SetMesh(hMesh);
      }
    }
  }
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestManyMeshesExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 1.0f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  ezResourceManager::ForceNoFallbackAcquisition(3);

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (iCurFrame > 3)
  {
    EZ_TEST_IMAGE(0, 150);

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////


void ezGameEngineTestApplication_Basics::SubTestSkyboxSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  ezTextureCubeResourceHandle hSkybox = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezBinMesh");

  // Skybox
  {
    ezGameObjectDesc go;
    go.m_LocalPosition.SetZero();

    ezGameObject* pObject;
    m_pWorld->CreateObject(go, pObject);

    ezSkyBoxComponent* pSkybox;
    m_pWorld->GetOrCreateComponentManager<ezSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);

    pSkybox->SetCubeMap(hSkybox);
  }

  // some foreground objects
  {
    ezInt32 dim = 5;

    for (ezInt32 z = -dim; z <= dim; ++z)
    {
      for (ezInt32 y = -dim; y <= dim; ++y)
      {
        for (ezInt32 x = -dim; x <= dim; ++x)
        {
          ezGameObjectDesc go;
          go.m_LocalPosition.Set(x * 10.0f, y * 10.0f, z * 10.0f);

          ezGameObject* pObject;
          m_pWorld->CreateObject(go, pObject);

          ezMeshComponent* pMesh;
          m_pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);

          pMesh->SetMesh(hMesh);
        }
      }
    }
  }
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestSkyboxExec(ezInt32 iCurFrame)
{
  ezResourceManager::ForceNoFallbackAcquisition(3);

  auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
  ezVec3 pos = ezVec3(iCurFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  pCamera->RotateGlobally(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(iCurFrame * 80.0f));

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (iCurFrame < 5)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(iCurFrame - 5, 150);

  if (iCurFrame < 8)
    return ezTestAppRun::Continue;

  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_Basics::SubTestDebugRenderingSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  ezRenderWorld::ResetFrameCounter();
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestDebugRenderingExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // line box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(25));
    ezDebugRenderer::DrawLineBox(m_pWorld.Borrow(), bbox, ezColor::HotPink, t);
  }

  // line box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -3, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(25));
    ezDebugRenderer::DrawLineBoxCorners(m_pWorld.Borrow(), bbox, 0.5f, ezColor::DeepPink, t);
  }

  // 2D Rect
  {
    ezDebugRenderer::Draw2DRectangle(m_pWorld.Borrow(), ezRectFloat(10, 50, 35, 15), 0.1f, ezColor::LawnGreen);
  }

  // Sphere
  {
    ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(8, -5, -4), 2);
    ezDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), sphere, ezColor::Tomato);
  }

  // Solid box
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, ezColor::BurlyWood);
  }

  // Text
  {
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), "Not 'a test\"", ezVec2I32(30, 10), ezColor::AntiqueWhite, 24);
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), "!@#$%^&*()_[]{}|", ezVec2I32(20, 200), ezColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    ezFrustum f = ezFrustum::MakeFromFOV(ezVec3(5, 7, 3), ezVec3(0, -1, 0), ezVec3(0, 0, 1), ezAngle::MakeFromDegree(30), ezAngle::MakeFromDegree(20), 0.1f, 5.0f);
    ezDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, ezColor::Cornsilk);
  }

  // Lines
  {
    ezHybridArray<ezDebugRenderer::Line, 4> lines;
    lines.PushBack(ezDebugRenderer::Line(ezVec3(3, -4, -4), ezVec3(4, -2, -3)));
    lines.PushBack(ezDebugRenderer::Line(ezVec3(4, -2, -3), ezVec3(2, 2, -2)));
    ezDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, ezColor::SkyBlue);
  }

  // Triangles
  {
    ezHybridArray<ezDebugRenderer::Triangle, 4> tris;
    tris.PushBack(ezDebugRenderer::Triangle(ezVec3(7, 0, 0), ezVec3(7, 2, 0), ezVec3(7, 2, 1)));
    tris.PushBack(ezDebugRenderer::Triangle(ezVec3(7, 3, 0), ezVec3(7, 1, 0), ezVec3(7, 3, 1)));
    ezDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), tris, ezColor::Gainsboro);
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  ezStringView sRendererName = ezGALDevice::GetDefaultDevice()->GetRenderer();
  const bool bRandomlyChangesLineThicknessOnDriverUpdate = sRendererName.IsEqual_NoCase("DX11") && ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia");

  EZ_TEST_LINE_IMAGE(0, bRandomlyChangesLineThicknessOnDriverUpdate ? 700 : 150);

  return ezTestAppRun::Quit;
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestDebugRenderingExec2(ezInt32 iCurFrame)
{
  {
    auto pCamera = ezDynamicCast<ezGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // Text
  {
    ezDebugRenderer::Draw2DText(m_pWorld.Borrow(), ezFmt("Frame# {}", ezRenderWorld::GetFrameCounter()), ezVec2I32(10, 10), ezColor::AntiqueWhite, 24);
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugTextPlacement::BottomLeft, "test", ezFmt("Frame# {}", ezRenderWorld::GetFrameCounter()));
    ezDebugRenderer::DrawInfoText(m_pWorld.Borrow(), ezDebugTextPlacement::BottomRight, "test", "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|");
  }

  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(0, 150);

  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_Basics::SubTestLoadSceneSetup()
{
  ezResourceManager::ForceNoFallbackAcquisition(3);
  ezRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/Lighting.ezBinScene").IgnoreResult();
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestLoadSceneExec(ezInt32 iCurFrame)
{
  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 1:
      EZ_TEST_IMAGE(0, 150);
      break;

    case 2:
      EZ_TEST_IMAGE(1, 150);
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

void ezGameEngineTestApplication_Basics::SubTestGoReferenceSetup()
{
  ezResourceManager::ForceNoFallbackAcquisition(3);
  ezRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/GoReferences.ezBinScene").IgnoreResult();
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestGoReferenceExec(ezInt32 iCurFrame)
{
  Run();
  if (ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 5:
      EZ_TEST_IMAGE(0, 100);
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
