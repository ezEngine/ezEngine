#include <PCH.h>
#include "Basics.h"
#include <Core/World/WorldDesc.h>
#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/WorldSerializer/WorldReader.h>

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
  AddSubTest("Debug Rendering", SubTests::ST_DebugRendering);
  AddSubTest("Load Scene", SubTests::ST_LoadScene);
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

  if (iIdentifier == SubTests::ST_DebugRendering)
  {
    m_pOwnApplication->SubTestDebugRenderingSetup();
    return EZ_SUCCESS;
  }

  if (iIdentifier == SubTests::ST_LoadScene)
  {
    m_pOwnApplication->SubTestLoadSceneSetup();
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

  if (iIdentifier == SubTests::ST_DebugRendering)
    return m_pOwnApplication->SubTestDebugRenderingExec(m_iFrame);

  if (iIdentifier == SubTests::ST_LoadScene)
    return m_pOwnApplication->SubTestLoadSceneExec(m_iFrame);

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
  {
    auto pCamera = GetGameStateForWorld(m_pWorld)->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  ezResourceManager::FinishLoadingOfResources();

  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  if (iCurFrame > 3)
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

  ezTextureCubeResourceHandle hSkybox = ezResourceManager::LoadResource<ezTextureCubeResource>("SharedData/Textures/Cubemap/ezLogo_Cube_DXT1_Mips_D.dds");
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
  pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
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

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_Basics::SubTestDebugRenderingSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestDebugRenderingExec(ezInt32 iCurFrame)
{
  {
    auto pCamera = GetGameStateForWorld(m_pWorld)->GetMainCamera();
    pCamera->SetCameraMode(ezCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    ezVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  }

  // line box
  {
    ezBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(25));
    ezDebugRenderer::DrawLineBox(m_pWorld, bbox, ezColor::HotPink, t);
  }

  // line box
  {
    ezBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(ezVec3(10, -3, 1), ezVec3(1, 2, 3));

    ezTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(25));
    ezDebugRenderer::DrawLineBoxCorners(m_pWorld, bbox, 0.5f, ezColor::DeepPink, t);
  }

  // 2D Rect
  {
    ezDebugRenderer::Draw2DRectangle(m_pWorld, ezRectFloat(10, 50, 35, 15), 0.1f, ezColor::LawnGreen);
  }

  // Sphere
  {
    ezBoundingSphere sphere;
    sphere.SetElements(ezVec3(8, -5, -4), 2);
    ezDebugRenderer::DrawLineSphere(m_pWorld, sphere, ezColor::Tomato);
  }

  // Solid box
  {
    ezBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(ezVec3(10, -5, 1), ezVec3(1, 2, 3));

    ezDebugRenderer::DrawSolidBox(m_pWorld, bbox, ezColor::BurlyWood);
  }

  // Text
  {
    ezDebugRenderer::DrawText(m_pWorld, "Not 'a test\"", ezVec2I32(30, 10), ezColor::AntiqueWhite, 24);
    ezDebugRenderer::DrawText(m_pWorld, "!@#$%^&*()_[]{}|", ezVec2I32(20, 200), ezColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    ezFrustum f;
    f.SetFrustum(ezVec3(5, 7, 3), ezVec3(0, -1, 0), ezVec3(0, 0, 1), ezAngle::Degree(30), ezAngle::Degree(20), 5.0f);
    ezDebugRenderer::DrawLineFrustum(m_pWorld, f, ezColor::Cornsilk);
  }

  // Lines
  {
    ezHybridArray<ezDebugRenderer::Line, 4> lines;
    lines.PushBack(ezDebugRenderer::Line(ezVec3(3, -4, -4), ezVec3(4, -2, -3)));
    lines.PushBack(ezDebugRenderer::Line(ezVec3(4, -2, -3), ezVec3(2, 2, -2)));
    ezDebugRenderer::DrawLines(m_pWorld, lines, ezColor::SkyBlue);
  }

  // Triangles
  {
    ezHybridArray<ezDebugRenderer::Triangle, 4> tris;
    tris.PushBack(ezDebugRenderer::Triangle(ezVec3(7, 0, 0), ezVec3(7, 2, 0), ezVec3(7, 2, 1)));
    tris.PushBack(ezDebugRenderer::Triangle(ezVec3(7, 3, 0), ezVec3(7, 1, 0), ezVec3(7, 3, 1)));
    ezDebugRenderer::DrawSolidTriangles(m_pWorld, tris, ezColor::Gainsboro);
  }

  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(150);

  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void ezGameEngineTestApplication_Basics::SubTestLoadSceneSetup()
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  {
    ezFileReader file;
    if (file.Open("GameEngineTest/Basics/AssetCache/Common/Lighting.ezObjectGraph").Succeeded())
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
      reader.InstantiateWorld(*m_pWorld);
      //reader.InstantiatePrefab(*m_pWorld, ezVec3(0, 2, 0), ezQuat::IdentityQuaternion(), ezVec3(0.1f));
    }
    else
    {
      ezLog::Error("Failed to read level");
    }
  }
}

ezTestAppRun ezGameEngineTestApplication_Basics::SubTestLoadSceneExec(ezInt32 iCurFrame)
{
  if (Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return ezTestAppRun::Continue;

  EZ_TEST_IMAGE(150);

  if (iCurFrame < 2)
    return ezTestAppRun::Continue;

  return ezTestAppRun::Quit;
}

