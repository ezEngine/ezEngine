#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include "Application.h"


void SampleApp::UpdateGame()
{
  m_pWorld->TransferThreadOwnership();

  m_pWorld->Update();

  UpdateGameInput();
}

void SampleApp::CreateGameLevel()
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Level");

  ezMeshComponentManager* pMeshCompMan = m_pWorld->CreateComponentManager<ezMeshComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;

  m_pWorld->CreateObject(obj, pObj);
  ezMeshComponent* pMesh;
  pMeshCompMan->CreateComponent(pMesh);
  pObj->AddComponent(pMesh);

  ezMaterialResourceHandle hMat = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Base.material");
  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/Sponza.ezmesh");

  //pMesh->SetMaterial(0, hMat);
  pMesh->SetMesh(hMesh);

  m_Camera.LookAt(ezVec3(0.0f, 0.0f, 0.0f), ezVec3(0.0f, 0.0f, -1.0f));
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);
  
  m_View.SetWorld(m_pWorld);
  m_View.SetLogicCamera(&m_Camera);
}

void SampleApp::DestroyGameLevel()
{
  m_pWorld->TransferThreadOwnership();
  EZ_DEFAULT_DELETE(m_pWorld);
}


