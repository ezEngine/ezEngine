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

  m_pWorld->m_pUserData = this;

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

  m_hPlayerShips[iPlayer] = m_pWorld->CreateObject(desc);

  ezGameObject* pGameObject = m_pWorld->GetObject(m_hPlayerShips[iPlayer]);

  ezComponentHandle hShipComponent = pShipManager->CreateComponent();
  pGameObject->AddComponent(hShipComponent);

  ShipComponent* pShipComponent = pShipManager->GetComponent(hShipComponent);
  pShipComponent->m_iPlayerIndex = iPlayer;


  ezComponentHandle hCollidableomponent = pCollidableManager->CreateComponent();
  pGameObject->AddComponent(hCollidableomponent);

  CollidableComponent* pCollidableComponent = pCollidableManager->GetComponent(hCollidableomponent);
  pCollidableComponent->m_fCollisionRadius = 1.0f;
}

void Level::CreateAsteroid()
{
  AsteroidComponentManager* pAsteroidManager     = m_pWorld->GetComponentManager<AsteroidComponentManager>();
  CollidableComponentManager* pCollidableManager = m_pWorld->GetComponentManager<CollidableComponentManager>();

  ezGameObjectDesc desc;
  desc.m_LocalPosition.x = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;
  desc.m_LocalPosition.y = (((rand() % 1000) / 999.0f) * 40.0f) - 20.0f;

  ezGameObjectHandle hAsteroid = m_pWorld->CreateObject(desc);

  ezGameObject* pGameObject = m_pWorld->GetObject(hAsteroid);

  ezComponentHandle hAsteroidComponent = pAsteroidManager->CreateComponent();
  pGameObject->AddComponent(hAsteroidComponent);

  AsteroidComponent* pAsteroidComponent = pAsteroidManager->GetComponent(hAsteroidComponent);
  
  ezComponentHandle hCollidableomponent = pCollidableManager->CreateComponent();
  pGameObject->AddComponent(hCollidableomponent);

  CollidableComponent* pCollidableComponent = pCollidableManager->GetComponent(hCollidableomponent);
  pCollidableComponent->m_fCollisionRadius = 1.0f;

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

  m_pLevel->SetupLevel("Asteroids");
}

void SampleGameApp::DestroyGameLevel()
{
  EZ_DEFAULT_DELETE(m_pLevel);
}

