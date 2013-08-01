#include "Level.h"
#include "Application.h"
#include <ShipComponent.h>
#include <ProjectileComponent.h>

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

  for (ezInt32 iPlayer = 0; iPlayer < MaxPlayers; ++iPlayer)
    CreatePlayerShip(iPlayer);
}

void Level::CreatePlayerShip(ezInt32 iPlayer)
{
  // CreateComponentManager can be called multiple times, it will just return the same manager again
  ShipComponentManager* pShipManager = m_pWorld->CreateComponentManager<ShipComponentManager>();
  ProjectileComponentManager* pProjectileManager = m_pWorld->CreateComponentManager<ProjectileComponentManager>();

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

