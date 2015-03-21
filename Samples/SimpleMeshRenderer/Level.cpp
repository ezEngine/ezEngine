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
  ezMeshComponent* pMesh;

  ezMaterialResourceHandle hMat = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Base.ezMaterial");
  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/Sponza.ezMesh");
  ezMeshResourceHandle hMeshTree = ezResourceManager::LoadResource<ezMeshResource>("Trees/Meshes/Tree5.ezMesh");

  // World Mesh
  {
    m_pWorld->CreateObject(obj, pObj);
    pMeshCompMan->CreateComponent(pMesh);
    pObj->AddComponent(pMesh);
    pMesh->SetMesh(hMesh);
  }

  // Tree Mesh
  {
    obj.m_LocalScaling.Set(70.0f);
    obj.m_LocalPosition.x = -500;
    m_pWorld->CreateObject(obj, pObj);
    pMeshCompMan->CreateComponent(pMesh);
    pObj->AddComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
  }

  // Tree Mesh
  {
    obj.m_LocalScaling.Set(60.0f);
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(75));
    obj.m_LocalPosition.x = 500;
    m_pWorld->CreateObject(obj, pObj);
    pMeshCompMan->CreateComponent(pMesh);
    pObj->AddComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
  }

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


