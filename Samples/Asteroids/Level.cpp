#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Graphics/Camera.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include "Level.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "AsteroidComponent.h"
#include "CollidableComponent.h"
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <GameEngine/Collection/CollectionResource.h>

extern const char* szPlayerActions[MaxPlayerActions];

Level::Level()
{
  m_pWorld = nullptr;
}

void Level::SetupLevel(ezWorld* pWorld)
{
  m_pWorld = pWorld;
  EZ_LOCK(m_pWorld->GetWriteMarker());

  // Load the collection that holds all assets and allows us to access them with nice names
  {
    m_hAssetCollection = ezResourceManager::LoadResource<ezCollectionResource>("{ c475e948-2e1d-4af0-b69b-d7c0bbad9130 }");

    ezResourceLock<ezCollectionResource> pCollection(m_hAssetCollection, ezResourceAcquireMode::NoFallback);
    pCollection->RegisterNames();
  }

  // Lights
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation.SetFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), -ezAngle::Degree(120.0f));

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);

  for (ezInt32 iAsteroid = 0; iAsteroid < MaxAsteroids; ++iAsteroid)
    CreateAsteroid();

  m_Camera.LookAt(ezVec3(0.0f, 0.0f, 100.0f), ezVec3(0.0f), ezVec3(0, 1, 0));
  m_Camera.SetCameraMode(ezCameraMode::OrthoFixedWidth, 45.0f, 0.0f, 500.0f);
}

void Level::UpdatePlayerInput(ezInt32 iPlayer)
{
  float fVal = 0.0f;

  ezGameObject* pShip = nullptr;
  if (!m_pWorld->TryGetObject(m_hPlayerShips[iPlayer], pShip))
    return;

  ShipComponent* pShipComponent = nullptr;
  if (!pShip->TryGetComponentOfBaseType(pShipComponent))
    return;

  ezVec3 vVelocity(0.0f);

  //const ezQuat qRot = pShip->GetLocalRotation();
  //const ezVec3 vShipDir = qRot * ezVec3(0, 1, 0);

  ezStringBuilder sControls[MaxPlayerActions];

  for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    sControls[iAction].Format("Player{0}_{1}", iPlayer, szPlayerActions[iAction]);


  if (ezInputManager::GetInputActionState("Game", sControls[0].GetData(), &fVal) != ezKeyState::Up)
  {
    //ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity += 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(0, 1, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != ezKeyState::Up)
  {
    //ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(0, -1, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != ezKeyState::Up)
  {
    //ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity += 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(-1, 0, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != ezKeyState::Up)
  {
    //ezVec3 vPos = pShip->GetLocalPosition();
    //vVelocity -= 0.1f * vShipDir * fVal;
    vVelocity += 0.1f * ezVec3(1, 0, 0) * fVal * 60.0f;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(3.0f * fVal * 60.0f));

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[5].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(-3.0f * fVal * 60.0f));

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (!vVelocity.IsZero())
    pShipComponent->SetVelocity(vVelocity);

  if (ezInputManager::GetInputActionState("Game", sControls[6].GetData(), &fVal) != ezKeyState::Up)
    pShipComponent->SetIsShooting(true);
  else
    pShipComponent->SetIsShooting(false);
}

void Level::CreatePlayerShip(ezInt32 iPlayer)
{
  // create one game object for the ship
  // then attach a ship component to that object

  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = -15 + iPlayer * 5.0f;

  ezGameObject* pGameObject = nullptr;
  m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc, pGameObject);

  {
    ezMeshComponent* pMeshComponent = nullptr;
    ezMeshComponent::CreateComponent(pGameObject, pMeshComponent);

    pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ShipMesh"));

    // this only works because the materials are part of the Asset Collection and get a name like this from there
    // otherwise we would need to have the GUIDs of the 4 different material assets available
    ezStringBuilder sMaterialName;
    sMaterialName.Format("MaterialPlayer{0}", iPlayer + 1);
    pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));
  }
  {
    ShipComponent* pShipComponent = nullptr;
    ezComponentHandle hShipComponent = ShipComponent::CreateComponent(pGameObject, pShipComponent);

    pShipComponent->m_iPlayerIndex = iPlayer;
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    ezComponentHandle hCollidableomponent = CollidableComponent::CreateComponent(pGameObject, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = 1.0f;
  }
}

void Level::CreateAsteroid()
{
  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;
  desc.m_LocalPosition.y = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;

  desc.m_LocalScaling = ezVec3(1.0f + ((rand() % 1000) / 999.0f));

  ezGameObject* pGameObject = nullptr;
  m_pWorld->CreateObject(desc, pGameObject);

  {
    ezMeshComponent* pMeshComponent = nullptr;
    ezMeshComponent::CreateComponent(pGameObject, pMeshComponent);

    pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("AsteroidMesh"));
  }
  {
    AsteroidComponent* pAsteroidComponent = nullptr;
    AsteroidComponent::CreateComponent(pGameObject, pAsteroidComponent);
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    CollidableComponent::CreateComponent(pGameObject, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = desc.m_LocalScaling.x;
  }
}


