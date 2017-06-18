#include <PCH.h>
#include "Basics.h"
#include <Core/World/WorldDesc.h>
#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

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
  AddSubTest("Many Meshes", SubTests::ST_ManyMeshes);
  AddSubTest("Skybox", SubTests::ST_Skybox);
}

ezResult ezGameEngineTestBasics::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (iIdentifier == SubTests::ST_ManyMeshes)
  {
    m_pOwnApplication->SubTestManyMeshesSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::ST_Skybox)
  {
    m_pOwnApplication->SubTestSkyboxSetup();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestBasics::RunSubTest(ezInt32 iIdentifier)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ST_ManyMeshes)
    return m_pOwnApplication->SubTestManyMeshesExec(m_iFrame);

  if (iIdentifier == SubTests::ST_Skybox)
    return m_pOwnApplication->SubTestSkyboxExec(m_iFrame);

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

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");

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
  ezResourceManager::FinishLoadingOfResources();

  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  if (iCurFrame > 5)
  {
    EZ_TEST_IMAGE(150);

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////


void ezGameEngineTestApplication_Basics::SubTestSkyboxSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  ezTextureCubeResourceHandle hSkybox = ezResourceManager::LoadResource<ezTextureCubeResource>("RendererTest/Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");

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
  ezResourceManager::FinishLoadingOfResources();

  auto pCamera = GetGameStateForWorld(m_pWorld)->GetMainCamera();
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 120.0f, 0.5f, 100.0f);
  ezVec3 pos = ezVec3(iCurFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  pCamera->RotateGlobally(ezAngle::Degree(0), ezAngle::Degree(0), ezAngle::Degree(iCurFrame * 80.0f));

  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  if (iCurFrame < 5)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(150);

  if (iCurFrame < 8)
    return ezTestAppRun::Continue;

  return ezTestAppRun::Quit;
}

