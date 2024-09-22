#include <GameEngineTest/GameEngineTestPCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererFoundation/Device/Device.h>

ezGameEngineTest::ezGameEngineTest() = default;
ezGameEngineTest::~ezGameEngineTest() = default;

ezResult ezGameEngineTest::GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber)
{
  ref_img.ResetAndCopy(m_pApplication->GetLastScreenshot());

  return EZ_SUCCESS;
}

ezResult ezGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return EZ_FAILURE;


  EZ_SUCCEED_OR_RETURN(ezRun_Startup(m_pApplication));

  if (ezGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11"))
  {
    if (ezGALDevice::HasDefaultDevice() && (ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics")))
    {
      // Use different images for comparison when running the D3D11 Reference Device
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (ezGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && ezGALDevice::HasDefaultDevice() && (ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon")))
    {
      // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
    }
    else
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan"))
  {
    if (ezGALDevice::HasDefaultDevice() && ezGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else
    {
      ezTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->RequestQuit();

    ezInt32 iSteps = 2;
    while (m_pApplication->Run() == ezApplication::Execution::Continue && iSteps > 0)
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

ezResult ezGameEngineTest::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  ezResourceManager::ForceNoFallbackAcquisition(3);

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


ezGameEngineTestApplication::ezGameEngineTestApplication(const char* szProjectDirName)
  : ezGameApplication("ezGameEngineTest", nullptr)
{
  m_pWorld = nullptr;
  m_sProjectDirName = szProjectDirName;
}


ezString ezGameEngineTestApplication::FindProjectDirectory() const
{
  return m_sAppProjectPath;
}

ezString ezGameEngineTestApplication::GetProjectDataDirectoryPath() const
{
  ezStringBuilder sProjectPath(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath(), "/", m_sProjectDirName);
  return sProjectPath;
}

ezResult ezGameEngineTestApplication::LoadScene(const char* szSceneFile)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();
  m_pWorld->GetRandomNumberGenerator().Initialize(42); // reset the RNG
  m_pWorld->GetClock().Reset(false);                   // reset the world clock

  ezFileReader file;

  if (file.Open(szSceneFile).Succeeded())
  {
    // File Header
    {
      ezAssetFileHeader header;
      EZ_SUCCEED_OR_RETURN(header.Read(file));

      char szSceneTag[16];
      file.ReadBytes(szSceneTag, sizeof(char) * 16);

      EZ_ASSERT_RELEASE(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid scene file");
    }

    ezWorldReader reader;
    EZ_SUCCEED_OR_RETURN(reader.ReadWorldDescription(file));
    reader.InstantiateWorld(*m_pWorld, nullptr);

    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Failed to load scene '{0}'", szSceneFile);
    return EZ_FAILURE;
  }
}

ezResult ezGameEngineTestApplication::BeforeCoreSystemsStartup()
{
  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  ezStringBuilder sProject;
  EZ_SUCCEED_OR_RETURN(ezFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sProject));
  m_sAppProjectPath = sProject;

  return EZ_SUCCESS;
}


void ezGameEngineTestApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  ezWorldDesc desc("GameEngineTestWorld");
  desc.m_uiRandomNumberGeneratorSeed = 42;

  m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);
  m_pWorld->GetClock().SetFixedTimeStep(ezTime::MakeFromSeconds(1.0 / 30.0));

  ActivateGameState(m_pWorld.Borrow(), {}, ezTransform::MakeIdentity());
}

void ezGameEngineTestApplication::BeforeHighLevelSystemsShutdown()
{
  m_pWorld = nullptr;

  SUPER::BeforeHighLevelSystemsShutdown();
}

void ezGameEngineTestApplication::StoreScreenshot(ezImage&& image, ezStringView sContext)
{
  // store this for image comparison purposes
  m_LastScreenshot.ResetAndMove(std::move(image));
}


void ezGameEngineTestApplication::Init_FileSystem_ConfigureDataDirs()
{
  SUPER::Init_FileSystem_ConfigureDataDirs();

  // additional data directories for the tests to work
  {
    ezFileSystem::SetSpecialDirectory("testout", ezTestFramework::GetInstance()->GetAbsOutputPath());

    ezStringBuilder sBaseDir = ">sdk/Data/Base/";
    ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());

    ezFileSystem::AddDataDirectory(">eztest/", "ImageComparisonDataDir", "imgout", ezDataDirUsage::AllowWrites).IgnoreResult();
    ezFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
  }
}

ezUniquePtr<ezGameStateBase> ezGameEngineTestApplication::CreateGameState()
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
  ezGameApplicationBase::GetGameApplicationBaseInstance()->TakeScreenshot();
}

void ezGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
