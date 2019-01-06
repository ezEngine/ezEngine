#include "Main.h"
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
#include <GameEngine/Components/InputComponent.h>
#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/Components/SliderComponent.h>
#include <GameEngine/Components/SpawnComponent.h>
#include <GameEngine/Components/TimedDeathComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <System/XBoxController/InputDeviceXBox.h>
ezInputDeviceXBox360 g_XboxInputDevice;
#endif

ezPlayerApplication::ezPlayerApplication()
    : ezGameApplication("ezPlayer", nullptr)
{
  m_pWorld = nullptr;
}

void ezPlayerApplication::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("player");

  ezGameApplication::BeforeCoreSystemsStartup();

  m_sSceneFile = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene", 0, "");
  EZ_ASSERT_ALWAYS(!m_sSceneFile.IsEmpty(),
                   "Scene file has not been specified. Use the -scene command followed by a full path to the ezBinaryScene file");

  m_sAppProjectPath = FindProjectDirectoryForScene(m_sSceneFile);
  EZ_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "No project directory could be found for scene file '{0}'", m_sSceneFile);
}


void ezPlayerApplication::AfterCoreSystemsStartup()
{
  DoProjectSetup();
  DoSetupGraphicsDevice();
  DoSetupDefaultResources();

  ezStartup::StartupHighLevelSystems();

  SetupLevel();

  ActivateGameState(m_pWorld.Borrow());
}

void ezPlayerApplication::BeforeCoreSystemsShutdown()
{
  DeactivateGameState();

  m_pWorld = nullptr;

  ezGameApplication::BeforeCoreSystemsShutdown();
}

void ezPlayerApplication::SetupLevel()
{
  EZ_LOG_BLOCK("SetupLevel", m_sSceneFile.GetData());

  ezStringBuilder sScenePath(m_sSceneFile);
  ezString sSceneFile = sScenePath.GetFileName();

  ezWorldDesc desc(sSceneFile);
  m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);

  EZ_LOCK(m_pWorld->GetWriteMarker());

  {
    ezFileReader file;
    if (file.Open(m_sSceneFile).Succeeded())
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
      // reader.InstantiatePrefab(*m_pWorld, ezVec3(0, 2, 0), ezQuat::IdentityQuaternion(), ezVec3(0.1f));
    }
    else
    {
      ezLog::Error("Could not read level '{0}'", m_sSceneFile);
    }
  }
}


EZ_APPLICATION_ENTRY_POINT(ezPlayerApplication);
