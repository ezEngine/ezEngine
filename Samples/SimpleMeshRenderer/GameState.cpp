#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Graphics/Camera.h>

#include <RendererCore/Meshes/MeshComponent.h>

#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>

#include "GameState.h"
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SimpleMeshRendererGameState, 1, ezRTTIDefaultAllocator<SimpleMeshRendererGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

SimpleMeshRendererGameState::SimpleMeshRendererGameState()
{
}

SimpleMeshRendererGameState::~SimpleMeshRendererGameState()
{

}

void SimpleMeshRendererGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Activate");

  ezGameState::OnActivation(pWorld);

  CreateGameLevel();
}

void SimpleMeshRendererGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Deactivate");

  DestroyGameLevel();

  ezGameState::OnDeactivation();
}

float SimpleMeshRendererGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 1.0f;
}

void SimpleMeshRendererGameState::CreateGameLevel()
{
  ezWorldDesc desc("Level");
  m_pMainWorld = GetApplication()->CreateWorld(desc);
  EZ_LOCK( m_pMainWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = m_pMainWorld->GetOrCreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = m_pMainWorld->GetOrCreateComponentManager<ezRotorComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;
  ezRotorComponent* pRotor;

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Sponza/Meshes/Sponza.ezMesh");
  ezMeshResourceHandle hMeshTree = ezResourceManager::LoadResource<ezMeshResource>("Trees/Meshes/Tree5.ezMesh");

  // World Mesh
  {
    obj.m_sName.Assign("Sponza");
    m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMesh);
    pObj->AttachComponent(pMesh);
  }

  // Tree Mesh
  {
    obj.m_sName.Assign("Rotating Tree");
    obj.m_LocalScaling.Set(0.5f);
    obj.m_LocalPosition.y = -5;
    m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
    pObj->AttachComponent(pMesh);

    pRotorCompMan->CreateComponent(pRotor);
    pRotor->m_fAnimationSpeed = 5.0f;
    pRotor->SetAnimatingAtStartup(true);
    pRotor->m_Axis = ezBasisAxis::PositiveZ;
    pObj->AttachComponent(pRotor);
  }

  // Tree Mesh
  {
    obj.m_sName.Assign("Tree");
    obj.m_LocalScaling.Set(0.7f);
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(75));
    obj.m_LocalPosition.y = 5;
    m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
    pObj->AttachComponent(pMesh);
  }

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));
    obj.m_LocalPosition.SetZero();
    obj.m_LocalRotation.SetIdentity();

    m_pMainWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(m_pMainWorld, pDirLight);
    pObj->AttachComponent(pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(m_pMainWorld, pAmbLight);
    pObj->AttachComponent(pAmbLight);
  }

  ChangeMainWorld(m_pMainWorld);
}

void SimpleMeshRendererGameState::DestroyGameLevel()
{
  GetApplication()->DestroyWorld(m_pMainWorld);
}

EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "SimpleMeshRenderer", ezGameApplicationType::StandAlone, "Data/Samples/SimpleMeshRenderer");


