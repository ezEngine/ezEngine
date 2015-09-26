#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/Camera.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Material/MaterialResource.h>

#include <GameUtils/Components/RotorComponent.h>

#include "GameState.h"
#include "Window.h"

void GameState::CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV)
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld, "Level");
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = m_pWorld->CreateComponentManager<ezRotorComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;
  ezRotorComponent* pRotor;

  ezMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezMeshResource>("Sponza/Meshes/Sponza.ezMesh");
  ezMeshResourceHandle hMeshTree = ezResourceManager::LoadResource<ezMeshResource>("Trees/Meshes/Tree5.ezMesh");

  // World Mesh
  {
    m_pWorld->CreateObject(obj, pObj);
    
    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMesh);
    pObj->AddComponent(pMesh);    
  }

  // Tree Mesh
  {
    obj.m_LocalScaling.Set(7.0f);
    obj.m_LocalPosition.y = -50;
    m_pWorld->CreateObject(obj, pObj);

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
    obj.m_LocalScaling.Set(6.0f);
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(75));
    obj.m_LocalPosition.y = 50;
    m_pWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pMesh);
    pMesh->SetMesh(hMeshTree);
    pObj->AddComponent(pMesh);    
  }

  ezVec3 vCameraPos = ezVec3(0.0f, 0.0f, 10.0f);

  ezCoordinateSystem coordSys;
  m_pWorld->GetCoordinateSystem(vCameraPos, coordSys);

  m_Camera.LookAt(vCameraPos, vCameraPos + coordSys.m_vForwardDir, coordSys.m_vUpDir);
  m_Camera.SetCameraMode(ezCamera::PerspectiveFixedFovY, 60.0f, 1.0f, 5000.0f);

  m_pView = ezRenderLoop::CreateView("Asteroids - View");
  ezRenderLoop::AddMainView(m_pView);

  ezGALRenderTagetSetup RTS;
  RTS.SetRenderTarget(0, hBackBuffer)
     .SetDepthStencilTarget(hDSV);

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass, RTS));
  m_pView->SetRenderPipeline(std::move(pRenderPipeline));

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pView->SetWorld(m_pWorld);
  m_pView->SetLogicCamera(&m_Camera);
}

void GameState::DestroyGameLevel()
{
  EZ_DEFAULT_DELETE(m_pWorld);
}


