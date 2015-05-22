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

void GameState::CreateGameLevelAndRenderPipeline(ezGALRenderTargetConfigHandle hRTConfig)
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld, "Level");
  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezMeshComponentManager* pMeshCompMan = m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = m_pWorld->CreateComponentManager<ezRotorComponentManager>();

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;
  ezRotorComponent* pRotor;

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
    pRotorCompMan->CreateComponent(pRotor);
    pRotor->m_fAnimationSpeed = 5.0f;
    pRotor->SetAnimatingAtStartup(true);
    pRotor->m_Axis = ezTransformComponentAxis::PosY;
    pObj->AddComponent(pRotor);
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

  m_pView = ezRenderLoop::CreateView("Asteroids - View");
  ezRenderLoop::AddMainView(m_pView);

  ezRenderPipeline* pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  pRenderPipeline->AddPass(EZ_DEFAULT_NEW(ezSimpleRenderPass, hRTConfig));
  m_pView->SetRenderPipeline(pRenderPipeline);

  ezSizeU32 size = m_pWindow->GetClientAreaSize();
  m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));

  m_pView->SetWorld(m_pWorld);
  m_pView->SetLogicCamera(&m_Camera);

  // Setup default resources
  {
    ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/Fallback_D.dds");
    ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");
    ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/Missing.ezMaterial");

    ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
    ezTextureResource::SetTypeMissingResource(hMissingTexture);
    ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  }
}

void GameState::DestroyGameLevel()
{
  /// \todo: we need some better cleanup mechanism here
  auto views = ezRenderLoop::GetMainViews();
  for (auto pView : views)
  {
    ezRenderPipeline* pRenderPipeline = pView->GetRenderPipeline();

    EZ_DEFAULT_DELETE(pRenderPipeline);

    EZ_DEFAULT_DELETE(pView);
  }

  ezRenderLoop::ClearMainViews();

  EZ_DEFAULT_DELETE(m_pWorld);
}


