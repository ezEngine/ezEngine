#include <GameEngineTest/GameEngineTestPCH.h>

#include "SkyTest.h"
#include <RendererCore/Components/AtmosphericScatteringComponent.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static ezGameEngineTestSky s_GameEngineTestSky;

namespace
{
  void CreateTestMeshes(ezWorld* pWorld, ezInt32 iDim, float fSpacing, const ezVec3& vOffset)
  {
    ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezBinMesh");
    ezMaterialResourceHandle hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ b93a6e80-5bf7-48f0-b7b3-2097d0133159 }"); // FullbrightWhite

    for (ezInt32 z = -iDim; z <= iDim; ++z)
    {
      for (ezInt32 y = -iDim; y <= iDim; ++y)
      {
        for (ezInt32 x = -iDim; x <= iDim; ++x)
        {
          ezGameObjectDesc go;
          go.m_LocalPosition = vOffset + ezVec3(x * fSpacing, y * fSpacing, z * fSpacing);

          ezGameObject* pObject;
          pWorld->CreateObject(go, pObject);

          ezMeshComponent* pMesh;
          pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);
          pMesh->SetMaterial(0, hMaterial);
          pMesh->SetMesh(hMesh);
        }
      }
    }
  }

  /// The sun light travels along the object's X axis, rotating around Y by the elevation makes it point downwards.
  ezQuat GetSunRotation(float fElevationDegree)
  {
    return ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(fElevationDegree));
  }
} // namespace

const char* ezGameEngineTestSky::GetTestName() const
{
  return "Sky Tests";
}

ezGameEngineTestApplication* ezGameEngineTestSky::CreateApplication()
{
  // only uses assets from the Base data directory, the Basics project is just a place to run from
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "Basics");
  return m_pOwnApplication;
}

void ezGameEngineTestSky::SetupSubTests()
{
  AddSubTest("Skybox", SubTests::Skybox);
  AddSubTest("Skybox Fog", SubTests::SkyboxFog);
  AddSubTest("Atmospheric Scattering", SubTests::AtmosphericScattering);
}

ezResult ezGameEngineTestSky::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  {
    ezWorld* pWorld = m_pOwnApplication->GetWorld();
    EZ_LOCK(pWorld->GetWriteMarker());
    pWorld->Clear();
  }

  switch (iIdentifier)
  {
    case SubTests::Skybox:
      SetupSkybox();
      return EZ_SUCCESS;

    case SubTests::SkyboxFog:
      SetupSkyboxFog();
      return EZ_SUCCESS;

    case SubTests::AtmosphericScattering:
      SetupAtmosphericScattering();
      return EZ_SUCCESS;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestSky::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ++m_iFrame;

  ezResourceManager::ForceNoFallbackAcquisition(3);

  switch (iIdentifier)
  {
    case SubTests::Skybox:
      return RunSkybox();

    case SubTests::SkyboxFog:
      return RunSkyboxFog();

    case SubTests::AtmosphericScattering:
      return RunAtmosphericScattering();

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezTestAppRun::Quit;
}

ezCamera* ezGameEngineTestSky::GetCamera() const
{
  return ezDynamicCast<ezGameState*>(m_pOwnApplication->GetActiveGameState())->GetMainCamera();
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestSky::SetupSkybox()
{
  ezWorld* pWorld = m_pOwnApplication->GetWorld();
  EZ_LOCK(pWorld->GetWriteMarker());

  ezTextureCubeResourceHandle hSkybox = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");

  ezGameObjectDesc go;
  ezGameObject* pObject;
  pWorld->CreateObject(go, pObject);

  ezSkyBoxComponent* pSkybox;
  pWorld->GetOrCreateComponentManager<ezSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);
  pSkybox->SetCubeMap(hSkybox);

  CreateTestMeshes(pWorld, 5, 10.0f, ezVec3::MakeZero());
}

ezTestAppRun ezGameEngineTestSky::RunSkybox()
{
  ezCamera* pCamera = GetCamera();
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
  ezVec3 pos = ezVec3(m_iFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  pCamera->RotateGlobally(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(m_iFrame * 80.0f));

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (m_iFrame < 5)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(m_iFrame - 5, 150);

  if (m_iFrame < 8)
    return ezTestAppRun::Continue;

  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestSky::SetupSkyboxFog()
{
  ezWorld* pWorld = m_pOwnApplication->GetWorld();
  EZ_LOCK(pWorld->GetWriteMarker());

  ezTextureCubeResourceHandle hSkybox = ezResourceManager::LoadResource<ezTextureCubeResource>("Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");

  ezGameObjectDesc go;
  ezGameObject* pObject;
  pWorld->CreateObject(go, pObject);

  ezSkyBoxComponent* pSkybox;
  pWorld->GetOrCreateComponentManager<ezSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);
  pSkybox->SetCubeMap(hSkybox);
  pSkybox->SetUseFog(true);
  pSkybox->SetVirtualDistance(50.0f);

  // without height falloff the fog only depends on the distance to the camera, so the sky has to look the same at every camera position
  ezFogComponent* pFog;
  pWorld->GetOrCreateComponentManager<ezFogComponentManager>()->CreateComponent(pObject, pFog);
  pFog->SetColor(ezColor::LightGray);
  pFog->SetDensity(1.0f);
  pFog->SetHeightFalloff(0.0f);
  pFog->SetStartDistance(0.0f);
}

ezTestAppRun ezGameEngineTestSky::RunSkyboxFog()
{
  // first at the origin, then far away from it, the sky fog must be identical in both images
  const ezVec3 pos = m_iFrame < 6 ? ezVec3::MakeZero() : ezVec3(3000.0f, -2000.0f, 0.0f);

  ezCamera* pCamera = GetCamera();
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 90.0f, 1.0f, 100.0f);
  pCamera->LookAt(pos, pos + ezVec3(1, 0, 0.3f), ezVec3(0, 0, 1));

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (m_iFrame == 5)
    EZ_TEST_IMAGE(0, 150);

  if (m_iFrame == 8)
  {
    EZ_TEST_IMAGE(1, 150);
    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestSky::SetupAtmosphericScattering()
{
  ezWorld* pWorld = m_pOwnApplication->GetWorld();
  EZ_LOCK(pWorld->GetWriteMarker());

  {
    ezGameObjectDesc go;
    go.m_LocalRotation = GetSunRotation(60.0f);
    go.m_bDynamic = true;

    ezGameObject* pObject;
    m_hSunObject = pWorld->CreateObject(go, pObject);

    ezAtmosphericScatteringComponent* pAtmosphere;
    pWorld->GetOrCreateComponentManager<ezAtmosphericScatteringComponentManager>()->CreateComponent(pObject, pAtmosphere);
    pAtmosphere->SetUseFog(false);
    // the default albedo makes the ground brighter than the white meshes, there is no tone mapping in the test pipeline
    pAtmosphere->SetGroundAlbedo(ezColor(0.05f, 0.05f, 0.05f));
  }

  // a row of meshes in front of the camera, the sky must not be drawn over them
  {
    ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezBinMesh");
    ezMaterialResourceHandle hMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ b93a6e80-5bf7-48f0-b7b3-2097d0133159 }"); // FullbrightWhite

    for (ezInt32 y = -2; y <= 2; ++y)
    {
      ezGameObjectDesc go;
      go.m_LocalPosition.Set(-20.0f, y * 8.0f, 0.0f);

      ezGameObject* pObject;
      pWorld->CreateObject(go, pObject);

      ezMeshComponent* pMesh;
      pWorld->GetOrCreateComponentManager<ezMeshComponentManager>()->CreateComponent(pObject, pMesh);
      pMesh->SetMaterial(0, hMaterial);
      pMesh->SetMesh(hMesh);
    }
  }
}

ezTestAppRun ezGameEngineTestSky::RunAtmosphericScattering()
{
  // The sun is lowered while the component stays alive, so the lookup tables have to be updated in place.
  // Each step gets a few frames, the image is taken at the end of it.
  constexpr ezInt32 iFramesPerStep = 4;
  constexpr float fElevations[] = {60.0f, 15.0f, 3.0f};
  constexpr ezInt32 iNumSteps = EZ_ARRAY_SIZE(fElevations);

  const ezInt32 iStep = ezMath::Min(m_iFrame / iFramesPerStep, iNumSteps - 1);

  {
    ezWorld* pWorld = m_pOwnApplication->GetWorld();
    EZ_LOCK(pWorld->GetWriteMarker());

    ezGameObject* pObject = nullptr;
    if (pWorld->TryGetObject(m_hSunObject, pObject))
    {
      pObject->SetLocalRotation(GetSunRotation(fElevations[iStep]));
    }
  }

  // looking towards the sun, which is in the -X direction
  ezCamera* pCamera = GetCamera();
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 90.0f, 1.0f, 1000.0f);
  const ezVec3 pos(0, 0, 2.0f);
  pCamera->LookAt(pos, pos + ezVec3(-1, 0, 0.25f), ezVec3(0, 0, 1));

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
    return ezTestAppRun::Quit;

  if (m_iFrame % iFramesPerStep == iFramesPerStep - 1)
  {
    EZ_TEST_IMAGE(iStep, 150);

    if (iStep == iNumSteps - 1)
      return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}
