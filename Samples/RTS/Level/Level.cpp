#include <PCH.h>
#include <RTS/Level.h>
#include <RTS/General/Application.h>
#include <RTS/Components/UnitComponent.h>
#include <RTS/Components/RevealerComponent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

Level::Level()
{
  m_pWorld = nullptr;
}

Level::~Level()
{
  EZ_DEFAULT_DELETE(m_pWorld);
}

void Level::SetupLevel()
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Level");
  m_pWorld->SetUserData(this);

  CreateRandomLevel();
}

void Level::CreateComponentManagers()
{
  UnitComponentManager* pUnitManager = m_pWorld->CreateComponentManager<UnitComponentManager>();
  RevealerComponentManager* pRevealerManager = m_pWorld->CreateComponentManager<RevealerComponentManager>();
  FollowPathSteeringComponentManager* pFollowPathSteeringManager = m_pWorld->CreateComponentManager<FollowPathSteeringComponentManager>();
  AvoidObstacleSteeringComponentManager* pAvoidObstacleSteeringManager = m_pWorld->CreateComponentManager<AvoidObstacleSteeringComponentManager>();
  ObstacleComponentManager* pObstacleManager = m_pWorld->CreateComponentManager<ObstacleComponentManager>();
  
}

void Level::Update()
{
  static ezTime LastVisUpdate = ezClock::Get()->GetAccumulatedTime();

  const ezTime UpdateInterval = ezTime::Seconds(1.0 / 20);

  if (ezClock::Get()->GetAccumulatedTime() - LastVisUpdate > UpdateInterval)
  {
    LastVisUpdate += UpdateInterval;

    for (ezUInt32 i = 0; i < m_GameGrid.GetNumCells(); ++i)
    {
      if (m_GameGrid.GetCell(i).m_uiVisibility > 1)
        --m_GameGrid.GetCell(i).m_uiVisibility;
    }
  }

  for (ezUInt32 i = 0; i < m_GameGrid.GetNumCells(); ++i)
  {
    m_GameGrid.GetCell(i).m_iThreat = 0;
  }

  m_pWorld->Update();

}


