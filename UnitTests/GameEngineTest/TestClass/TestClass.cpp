#include <PCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

ezGameEngineTest::ezGameEngineTest() = default;
 ezGameEngineTest::~ezGameEngineTest() = default;

ezResult ezGameEngineTest::GetImage(ezImage& img)
{
  img = m_pApplication->GetLastScreenshot();

  return EZ_SUCCESS;
}

ezResult ezGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return EZ_FAILURE;

  ezRun_Startup(m_pApplication);

  return EZ_SUCCESS;
}

ezResult ezGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->RequestQuit();

    ezInt32 iSteps = 2;
    while (m_pApplication->Run() == ezApplication::Continue && iSteps > 0)
    {
      --iSteps;
    }

    ezRun_Shutdown(m_pApplication);

    EZ_DEFAULT_DELETE(m_pApplication);

    if (iSteps == 0)
      return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


ezGameEngineTestApplication::ezGameEngineTestApplication(const char* szProjectDirName)
    : ezGameApplication("ezGameEngineTest", ezGameApplicationType::StandAlone, nullptr)
{
  m_pWorld = nullptr;
  m_sProjectDirName = szProjectDirName;
}


ezString ezGameEngineTestApplication::FindProjectDirectory() const
{
  return m_sAppProjectPath;
}

ezResult ezGameEngineTestApplication::LoadScene(const char* szSceneFile)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();

  ezFileReader file;

  if (file.Open(szSceneFile).Succeeded())
  {
    // File Header
    {
      ezAssetFileHeader header;
      header.Read(file);

      char szSceneTag[16];
      file.ReadBytes(szSceneTag, sizeof(char) * 16);

      EZ_ASSERT_RELEASE(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid scene file");
    }

    ezWorldReader reader;
    reader.ReadWorldDescription(file);
    reader.InstantiateWorld(*m_pWorld, nullptr);

    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Failed to load scene '{0}'", szSceneFile);
    return EZ_FAILURE;
  }
}

void ezGameEngineTestApplication::BeforeCoreStartup()
{
  ezGameApplication::BeforeCoreStartup();

  ezStringBuilder sProjectPath(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath(), "/", m_sProjectDirName);

  ezStringBuilder sProject;
  ezFileSystem::ResolveSpecialDirectory(sProjectPath, sProject);
  m_sAppProjectPath = sProject;
}


void ezGameEngineTestApplication::AfterCoreStartup()
{
  DoProjectSetup();
  DoSetupGraphicsDevice();
  DoSetupDefaultResources();

  ezStartup::StartupEngine();

  ezWorldDesc desc("GameEngineTestWorld");
  m_pWorld = CreateWorld(desc);
  m_pWorld->GetClock().SetFixedTimeStep(ezTime::Seconds(1.0 / 30.0));

  CreateGameStateForWorld(m_pWorld);

  ActivateAllGameStates();
}

void ezGameEngineTestApplication::BeforeCoreShutdown()
{
  GetGameApplicationInstance()->DestroyWorld(m_pWorld);

  ezGameApplication::BeforeCoreShutdown();
}

void ezGameEngineTestApplication::DoSaveScreenshot(ezImage& image)
{
  // store this for image comparison purposes
  m_LastScreenshot = image;
}


void ezGameEngineTestApplication::DoSetupDataDirectories()
{
  ezGameApplication::DoSetupDataDirectories();

  // additional data directories for the tests to work
  {
    ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

    ezStringBuilder sBaseDir = ">sdk/Data/Base/";
    ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());

    ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezFileSystem::AllowWrites);
    ezFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir");
  }
}

ezGameState* ezGameEngineTestApplication::CreateCustomGameStateForWorld(ezWorld* pWorld)
{
  return EZ_DEFAULT_NEW(ezGameEngineTestGameState);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameEngineTestGameState, 1, ezRTTIDefaultAllocator<ezGameEngineTestGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezGameEngineTestGameState::ProcessInput()
{
  // Do nothing, user input should be ignored

  // trigger taking a screenshot every frame, for image comparison purposes
  GetApplication()->TakeScreenshot();
}

ezGameState::Priority ezGameEngineTestGameState::DeterminePriority(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return ezGameState::Priority::Default;
}

void ezGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
