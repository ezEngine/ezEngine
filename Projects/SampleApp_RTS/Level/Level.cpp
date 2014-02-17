#include <PCH.h>
#include <SampleApp_RTS/Level.h>
#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/Components/UnitComponent.h>
#include <SampleApp_RTS/Components/RevealerComponent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

Level::Level()
{
  m_pWorld = NULL;
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

class DummyTask : public ezTask
{
private:
  virtual void Execute() EZ_OVERRIDE
  {
    ezThreadUtils::Sleep(1);
  }

public:
  static void OnTaskFinished(ezTask* pTask, void* pPassThrough)
  {
    EZ_DEFAULT_DELETE(pTask);
  }
};

void Level::Update()
{
  for (ezUInt32 i = 0; i < 50 + (ezUInt32) rand() % 300; ++i)
  {
    DummyTask* pTask = EZ_DEFAULT_NEW(DummyTask);
    pTask->SetOnTaskFinished(DummyTask::OnTaskFinished);

    ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::ThisFrame);
  }

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


