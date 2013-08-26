#include "Level.h"
#include "Application.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "AsteroidComponent.h"
#include "CollidableComponent.h"

Level::Level()
{
  m_pWorld = NULL;
}

Level::~Level()
{
  EZ_DEFAULT_DELETE(m_pWorld);
}

void Level::SetupLevel(const char* szLevelName)
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)(szLevelName);

  ShipComponentManager* pShipManager = m_pWorld->CreateComponentManager<ShipComponentManager>();
  ProjectileComponentManager* pProjectileManager = m_pWorld->CreateComponentManager<ProjectileComponentManager>();
  AsteroidComponentManager* pAsteroidManager = m_pWorld->CreateComponentManager<AsteroidComponentManager>();
  CollidableComponentManager* pCollidableManager = m_pWorld->CreateComponentManager<CollidableComponentManager>();


  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);

  for (ezInt32 iAsteroid = 0; iAsteroid < MaxAsteroids; ++iAsteroid)
    CreateAsteroid();
}

void Level::CreatePlayerShip(ezInt32 iPlayer)
{
  ShipComponentManager* pShipManager = m_pWorld->GetComponentManager<ShipComponentManager>();
  CollidableComponentManager* pCollidableManager = m_pWorld->GetComponentManager<CollidableComponentManager>();

  // create one game object for the ship
  // then attach a ship component to that object

  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = -15 + iPlayer * 5.0f;

  ezGameObject* pGameObject = NULL;
  m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc, pGameObject);

  {
    ShipComponent* pShipComponent = NULL;
    ezComponentHandle hShipComponent = pShipManager->CreateComponent(pShipComponent);
  
    pShipComponent->m_iPlayerIndex = iPlayer;

    pGameObject->AddComponent(hShipComponent);
  }
  {
    CollidableComponent* pCollidableComponent = NULL;
    ezComponentHandle hCollidableomponent = pCollidableManager->CreateComponent(pCollidableComponent);

    pCollidableComponent->m_fCollisionRadius = 1.0f;

    pGameObject->AddComponent(hCollidableomponent);
  }
}

void Level::CreateAsteroid()
{
  AsteroidComponentManager* pAsteroidManager     = m_pWorld->GetComponentManager<AsteroidComponentManager>();
  CollidableComponentManager* pCollidableManager = m_pWorld->GetComponentManager<CollidableComponentManager>();

  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;
  desc.m_LocalPosition.y = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;

  const float fRadius = 1.0f + ((rand() % 1000) / 999.0f);

  ezGameObject* pGameObject = NULL;
  ezGameObjectHandle hAsteroid = m_pWorld->CreateObject(desc, pGameObject);

  {
    AsteroidComponent* pAsteroidComponent = NULL;
    ezComponentHandle hAsteroidComponent = pAsteroidManager->CreateComponent(pAsteroidComponent);

    pAsteroidComponent->m_fRadius = fRadius;
  
    pGameObject->AddComponent(hAsteroidComponent);
  }
  {
    CollidableComponent* pCollidableComponent = NULL;
    ezComponentHandle hCollidableomponent = pCollidableManager->CreateComponent(pCollidableComponent);
  
    pCollidableComponent->m_fCollisionRadius = fRadius;
    
    pGameObject->AddComponent(hCollidableomponent);
  }
}

void Level::Update()
{
  m_pWorld->Update();

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    UpdatePlayerInput(iPlayer);
}



void SampleGameApp::CreateGameLevel()
{
  m_pLevel = EZ_DEFAULT_NEW(Level);

  m_pLevel->SetupLevel("Asteroids - World");
}

void SampleGameApp::DestroyGameLevel()
{
  EZ_DEFAULT_DELETE(m_pLevel);
}

