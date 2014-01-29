#include <SampleApp_RTS/Level.h>
#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/Components/UnitComponent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

ezUInt32 GameCellData::s_uiVisitCounter = 0;

Level::Level()
{
  m_pWorld = NULL;
}

Level::~Level()
{
  //EZ_DEFAULT_DELETE(m_pWorld->GetComponentManager<UnitComponentManager>());

  EZ_DEFAULT_DELETE(m_pWorld);
}

bool Level::IsSameCellType(ezUInt32 uiCell1, ezUInt32 uiCell2, void* pPassThrough)
{
  const GameGrid* pGrid = (const GameGrid*) pPassThrough;

  return pGrid->GetCell(uiCell1).m_iCellType == pGrid->GetCell(uiCell2).m_iCellType;
}

bool Level::IsCellBlocked(ezUInt32 uiCell, void* pPassThrough)
{
  const GameGrid* pGrid = (const GameGrid*) pPassThrough;

  return pGrid->GetCell(uiCell).m_iCellType == 1;
}

ezCallbackResult::Enum Level::SetPointBlocking(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  GameGrid* pGrid = (GameGrid*) pPassThrough;

  if (x < 0 || y < 0 || x >= pGrid->GetGridWidth() || y >= pGrid->GetGridHeight())
    return ezCallbackResult::Continue;

  pGrid->GetCell(ezVec2I32(x, y)).m_iCellType = 1;

  return ezCallbackResult::Continue;
}

void Level::SetupLevel()
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Level");
  m_pWorld->SetUserData(this);

  m_GameGrid.CreateGrid(250, 250);
  m_GameGrid.SetWorldSpaceDimensions(ezVec3(-25.0f, 0, -25.0f), ezVec3(1.0f, 2.0f, 1.0f));

  for (ezUInt32 z = 0; z < m_GameGrid.GetGridHeight(); ++z)
  {
    for (ezUInt32 x = 0; x < m_GameGrid.GetGridWidth(); ++x)
    {
      if (rand() % 400 == 1)
      {
        ez2DGridUtils::RasterizeBlob(x, z, (ez2DGridUtils::ezBlobType) (ez2DGridUtils::Point1x1 + rand() % 8), SetPointBlocking, &m_GameGrid);
      }
    }
  }

  {
    EZ_LOG_BLOCK("Creating Navmesh from 2D Grid");

    ezStopwatch s;
    m_Navmesh.CreateFromGrid(m_GameGrid, IsSameCellType, &m_GameGrid, IsCellBlocked, &m_GameGrid);
    ezLog::Info("Time Taken: %.2f msec", s.Checkpoint().GetMilliseconds());

    ezLog::Info("Grid Cells: %i (%i * %i)", m_GameGrid.GetNumCells(), m_GameGrid.GetGridWidth(), m_GameGrid.GetGridHeight());
    ezLog::Info("Navmesh Cells: %i (%.2f%%)", m_Navmesh.GetNumConvexAreas(), (float) m_Navmesh.GetNumConvexAreas() / (float) m_GameGrid.GetNumCells() * 100.0f);
  }

  CreateComponentManagers();

  CreateUnit(UnitType::Default, ezVec3(0, 1, 0));

  //CreateUnit(UnitType::Default, ezVec3(5, 1, 0));

  //CreateUnit(UnitType::Default, ezVec3(3, 1, 0));
}

void Level::CreateComponentManagers()
{
  UnitComponentManager* pUnitManager = m_pWorld->CreateComponentManager<UnitComponentManager>();

}


void Level::Update()
{
  for (ezUInt32 i = 0; i < m_GameGrid.GetNumCells(); ++i)
  {
    m_GameGrid.GetCell(i).m_bOccupied = false;
  }

  m_pWorld->Update();

}


