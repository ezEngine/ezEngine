#include <PCH.h>
#include "Basics.h"
#include <Core/World/WorldDesc.h>
#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

static ezGameEngineTestBasics s_GameEngineTestBasics;

const char* ezGameEngineTestBasics::GetTestName() const
{
  return "Basic Engine Tests";
}


ezGameEngineTestApplication* ezGameEngineTestBasics::CreateApplication()
{
  return EZ_DEFAULT_NEW(ezGameEngineTestApplication_Basics);
}

void ezGameEngineTestBasics::SetupSubTests()
{
  AddSubTest("Simple Scene", SubTests::ST_SimpleScene);
}

ezTestAppRun ezGameEngineTestBasics::RunSubTest(ezInt32 iIdentifier)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ST_SimpleScene)
    return SubtestSimpleScene();

  return ezTestAppRun::Quit;
}

ezResult ezGameEngineTestBasics::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;



  return EZ_SUCCESS;
}

ezResult ezGameEngineTestBasics::DeInitializeSubTest(ezInt32 iIdentifier)
{

  return EZ_SUCCESS;
}

ezTestAppRun ezGameEngineTestBasics::SubtestSimpleScene()
{
  ezResourceManager::FinishLoadingOfResources();

  if (m_pApplication->Run() == ezApplication::Quit)
    return ezTestAppRun::Quit;

  if (m_iFrame > 5)
  {
    EZ_TEST_IMAGE(150);

    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}


ezGameEngineTestApplication_Basics::ezGameEngineTestApplication_Basics()
  : ezGameEngineTestApplication("Basics")
{

}

void ezGameEngineTestApplication_Basics::SetupWorld()
{
  ezWorldDesc desc("GameEngineTestWorld");
  m_pWorld = CreateWorld(desc);

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh.ezMesh");

  EZ_LOCK(m_pWorld->GetWriteMarker());

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

