#include <AsteroidsPlugin/Components/AsteroidComponent.h>
#include <AsteroidsPlugin/Components/CollidableComponent.h>
#include <AsteroidsPlugin/Components/ProjectileComponent.h>
#include <AsteroidsPlugin/Components/ShipComponent.h>
#include <AsteroidsPlugin/GameState/Level.h>
#include <Core/Collection/CollectionResource.h>
#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

extern const char* szPlayerActions[MaxPlayerActions];

Level::Level()
{
  m_pWorld = nullptr;
}

void Level::SetupLevel(ezUniquePtr<ezWorld> pWorld)
{
  m_pWorld = std::move(pWorld);
  EZ_LOCK(m_pWorld->GetWriteMarker());

  // Load the collection that holds all assets and allows us to access them with nice names
  {
    m_hAssetCollection = ezResourceManager::LoadResource<ezCollectionResource>("{ c475e948-2e1d-4af0-b69b-d7c0bbad9130 }");

    ezResourceLock<ezCollectionResource> pCollection(m_hAssetCollection, ezResourceAcquireMode::BlockTillLoaded);
    pCollection->RegisterNames();
  }

  // Lights
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("DirLight");
    obj.m_LocalRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0.0f, 1.0f, 0.0f), -ezAngle::MakeFromDegree(120.0f));

    ezGameObject* pObj;
    m_pWorld->CreateObject(obj, pObj);

    // point and spot lights won't work with the orthographic camera
    ezDirectionalLightComponent* pDirLight;
    ezDirectionalLightComponent::CreateComponent(pObj, pDirLight);

    ezAmbientLightComponent* pAmbLight;
    ezAmbientLightComponent::CreateComponent(pObj, pAmbLight);
  }

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);

  for (ezInt32 iAsteroid = 0; iAsteroid < MaxAsteroids; ++iAsteroid)
    CreateAsteroid();
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

  ezStringBuilder sControls[MaxPlayerActions];

  for (ezInt32 iAction = 0; iAction < MaxPlayerActions; ++iAction)
    sControls[iAction].SetFormat("Player{0}_{1}", iPlayer, szPlayerActions[iAction]);


  if (ezInputManager::GetInputActionState("Game", sControls[0].GetData(), &fVal) != ezKeyState::Up)
  {
    vVelocity += ezVec3(0, 1, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[1].GetData(), &fVal) != ezKeyState::Up)
  {
    vVelocity += ezVec3(0, -1, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[2].GetData(), &fVal) != ezKeyState::Up)
  {
    vVelocity += ezVec3(-1, 0, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[3].GetData(), &fVal) != ezKeyState::Up)
  {
    vVelocity += ezVec3(1, 0, 0) * fVal;
  }

  if (ezInputManager::GetInputActionState("Game", sControls[4].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(3.0f * fVal * 60.0f));

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", sControls[5].GetData(), &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::MakeFromDegree(-3.0f * fVal * 60.0f));

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (!vVelocity.IsZero())
    pShipComponent->AddExternalForce(vVelocity * 15000.0f);

  if (ezInputManager::GetInputActionState("Game", sControls[6].GetData(), &fVal) != ezKeyState::Up)
    pShipComponent->SetIsShooting(true);
  else
    pShipComponent->SetIsShooting(false);
}

void Level::CreatePlayerShip(ezInt32 iPlayer)
{
  // create one game object for the ship
  // then attach a ship component to that object

  ezGameObject* pShipObject = nullptr;

  {
    ezGameObjectDesc desc;
    desc.m_bDynamic = true;
    desc.m_LocalPosition.x = -15 + iPlayer * 5.0f;
    m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc, pShipObject);
  }

  {
    // add a sub-object to place the ship mesh at the proper position
    ezGameObject* pShipMeshObj = nullptr;

    {
      ezGameObjectDesc desc;
      desc.m_hParent = pShipObject->GetHandle();
      desc.m_LocalPosition.x = -0.5f;
      m_pWorld->CreateObject(desc, pShipMeshObj);
    }

    ezMeshComponent* pMeshComponent = nullptr;
    ezMeshComponent::CreateComponent(pShipMeshObj, pMeshComponent);

    pMeshComponent->SetMesh(ezResourceManager::LoadResource<ezMeshResource>("ShipMesh"));

    // this only works because the materials are part of the Asset Collection and get a name like this from there
    // otherwise we would need to have the GUIDs of the 4 different material assets available
    ezStringBuilder sMaterialName;
    sMaterialName.SetFormat("MaterialPlayer{0}", iPlayer + 1);
    pMeshComponent->SetMaterial(0, ezResourceManager::LoadResource<ezMaterialResource>(sMaterialName));
  }
  {
    ShipComponent* pShipComponent = nullptr;
    ezComponentHandle hShipComponent = ShipComponent::CreateComponent(pShipObject, pShipComponent);

    pShipComponent->m_iPlayerIndex = iPlayer;
  }
  {
    CollidableComponent* pCollidableComponent = nullptr;
    CollidableComponent::CreateComponent(pShipObject, pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = 1.0f;
  }
}

void Level::CreateAsteroid()
{
  ezGameObjectDesc desc;
  desc.m_bDynamic = true;
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
