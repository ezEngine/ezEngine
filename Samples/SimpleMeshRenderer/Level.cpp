#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/Camera.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/TargetPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Material/MaterialResource.h>

#include <GameUtils/Components/RotorComponent.h>
#include <GameFoundation/GameApplication/GameApplication.h>

#include "GameState.h"

void SimpleMeshRendererGameState::CreateGameLevel()
{
  m_pMainWorld = GetApplication()->CreateWorld( "Level", true );
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
	  m_pMainWorld->CreateObject(obj, pObj);
    
    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMesh);
    pObj->AddComponent(pMesh);    
  }

  // Tree Mesh
  {
    obj.m_LocalScaling.Set(0.5f);
    obj.m_LocalPosition.y = -5;
	m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
    pObj->AddComponent(pMesh);
    
    pRotorCompMan->CreateComponent(pRotor);
    pRotor->m_fAnimationSpeed = 5.0f;
    pRotor->SetAnimatingAtStartup(true);
    pRotor->m_Axis = ezBasisAxis::PositiveZ;
    pObj->AddComponent(pRotor);
  }

  // Tree Mesh
  {
    obj.m_LocalScaling.Set(0.7f);
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(75));
    obj.m_LocalPosition.y = 5;
	m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
    pObj->AddComponent(pMesh);    
  }

  ChangeMainWorld(m_pMainWorld);
}

void SimpleMeshRendererGameState::DestroyGameLevel()
{
  GetApplication()->DestroyWorld(m_pMainWorld);
}


