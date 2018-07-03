#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Graphics/Camera.h>

#include <RendererCore/Meshes/MeshComponent.h>

#include <GameEngine/Components/RotorComponent.h>
#include <GameEngine/GameApplication/GameApplication.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <Core/Input/InputManager.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
#include <GameEngine/MixedReality/Components/SpatialAnchorComponent.h>
#include <GameEngine/MixedReality/Components/SrmRenderComponent.h>
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#include <WindowsMixedReality/SpatialMapping/SurfaceReconstructionMeshManager.h>
#endif

#include "GameState.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SimpleMeshRendererGameState, 1, ezRTTIDefaultAllocator<SimpleMeshRendererGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

SimpleMeshRendererGameState::SimpleMeshRendererGameState() = default;
SimpleMeshRendererGameState::~SimpleMeshRendererGameState() = default;

void SimpleMeshRendererGameState::ConfigureInputActions()
{
  // Tap
  {
    ezInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = false;
    cfg.m_sInputSlotTrigger[0] = ezInputSlot_Spatial_Hand1_Pressed;
    ezInputManager::SetInputActionConfig("Game", "AirTap", cfg, true);
  }
}

void SimpleMeshRendererGameState::ProcessInput()
{
  if (ezInputManager::GetInputActionState("Game", "AirTap") == ezKeyState::Pressed)
  {
    ezVec3 posP(0), posN(0), pos(0);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionPosX, &posP.x);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionPosY, &posP.y);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionPosZ, &posP.z);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionNegX, &posN.x);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionNegY, &posN.y);
    ezInputManager::GetInputSlotState(ezInputSlot_Spatial_Hand1_PositionNegZ, &posN.z);
    pos = posP - posN;

    ezLog::Dev("Air Tap: {0} | {1} | {2}", pos.x, pos.y, pos.z);

    MoveObjectToPosition(pos);
  }

  const char* szPressed = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::None, ezInputSlotFlags::None);

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (IsMixedRealityMode() && !ezStringUtils::IsNullOrEmpty(szPressed))
  {
    ezLog::Info("Pressed: {0}", szPressed);

    ezMixedRealityFramework::GetSingleton()->GetSpatialMappingManager().PullCurrentSurfaces();
  }
#endif

  SUPER::ProcessInput();
}

void SimpleMeshRendererGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Activate");

  SUPER::OnActivation(pWorld);

  CreateGameLevel();
}

void SimpleMeshRendererGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("SimpleMeshRendererGameState::Deactivate");

  DestroyGameLevel();

  SUPER::OnDeactivation();
}

ezGameState::Priority SimpleMeshRendererGameState::DeterminePriority(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return ezGameState::Priority::Default;
}

void SimpleMeshRendererGameState::CreateGameLevel()
{
  ezWorldDesc desc("Level");
  m_pMainWorld = GetApplication()->CreateWorld(desc);
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

  const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");

  ezMeshComponentManager* pMeshCompMan = m_pMainWorld->GetOrCreateComponentManager<ezMeshComponentManager>();
  ezRotorComponentManager* pRotorCompMan = m_pMainWorld->GetOrCreateComponentManager<ezRotorComponentManager>();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezSpatialAnchorComponentManager* pAnchorMan = m_pMainWorld->GetOrCreateComponentManager<ezSpatialAnchorComponentManager>();
  ezSrmRenderComponentManager* pSrmCompMan = m_pMainWorld->GetOrCreateComponentManager<ezSrmRenderComponentManager>();
  ezSrmRenderComponent* pSrmRenderComp;
#endif

  ezGameObjectDesc obj;
  ezGameObject* pObj;
  ezMeshComponent* pMesh;
  //ezRotorComponent* pRotor;

  ezMeshResourceHandle hMeshSponza = ezResourceManager::LoadResource<ezMeshResource>("Sponza/Sponza.ezMesh");
  ezMeshResourceHandle hMeshBarrel = ezResourceManager::LoadResource<ezMeshResource>("{ c227019a-92d3-4bf5-b391-9320e11ca7ff }");
  ezMeshResourceHandle hMeshTree = ezResourceManager::LoadResource<ezMeshResource>("{ 5bbea795-0358-4dd8-86a9-c277f7a59f53 }");


#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  // SRM Mesh
  {
    obj.m_sName.Assign("SRM");
    m_pMainWorld->CreateObject(obj, pObj);
    pSrmCompMan->CreateComponent(pObj, pSrmRenderComp);
  }
#endif

  obj.m_Tags.Set(tagCastShadows);

  // Sponza Mesh
  {
    obj.m_sName.Assign("Sponza");
    obj.m_LocalScaling.Set(1.0f);
    obj.m_LocalPosition.z = -1;
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(90));
    m_hTree = m_pMainWorld->CreateObject(obj, pObj);

    pMeshCompMan->CreateComponent(pObj, pMesh);
    pMesh->SetMesh(hMeshSponza);

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
    ezSpatialAnchorComponent* pComp = nullptr;
    pAnchorMan->CreateComponent(pObj, pComp);
    pComp->SetPersistentAnchorName("MrAnchor");
#endif
  }

  //{
  //  obj.m_sName.Assign("Barrel without Anchor");
  //  obj.m_LocalScaling.Set(1.0f);
  //  obj.m_LocalPosition.x = 0.0f;
  //  m_pMainWorld->CreateObject(obj, pObj);

  //  pMeshCompMan->CreateComponent(pObj, pMesh);
  //  pMesh->SetMesh(hMeshBarrel); 
  //}

  // Tree Mesh
  //{
  //  obj.m_sName.Assign("Tree");
  //  obj.m_LocalScaling.Set(0.5f);
  //  obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(75));
  //  obj.m_LocalPosition.x = 5;
  //  m_pMainWorld->CreateObject(obj, pObj); 

  //  pMeshCompMan->CreateComponent(pObj, pMesh);
  //  pMesh->SetMesh(hMeshTree);
  //}

  // Lights
  {
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), ezAngle::Degree(60.0f));
    obj.m_LocalPosition.SetZero();

    m_pMainWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);
    //pDirLight->SetCastShadows(true);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }

  ChangeMainWorld(m_pMainWorld);
}

void SimpleMeshRendererGameState::DestroyGameLevel()
{
  GetApplication()->DestroyWorld(m_pMainWorld);
}


void SimpleMeshRendererGameState::MoveObjectToPosition(const ezVec3& pos)
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());

  ezGameObject* pObject = nullptr;
  if (!m_pMainWorld->TryGetObject(m_hTree, pObject))
    return;

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  if (IsMixedRealityMode())
  {
    ezSpatialAnchorComponent* pComp = nullptr;
    if (pObject->TryGetComponentOfBaseType<ezSpatialAnchorComponent>(pComp))
    {
      pComp->RecreateAnchorAt(ezTransform(pos));
    }
  }
#endif
}


// This application supports being compiled for both modes
#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "SimpleMeshRenderer", ezGameApplicationType::StandAloneMixedReality, "Data/Samples/SimpleMeshRenderer");
#else
EZ_APPLICATION_ENTRY_POINT(ezGameApplication, "SimpleMeshRenderer", ezGameApplicationType::StandAlone, "Data/Samples/SimpleMeshRenderer");
#endif


