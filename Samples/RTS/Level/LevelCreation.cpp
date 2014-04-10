#include <PCH.h>
#include <RTS/Level.h>
#include <RTS/General/Application.h>
#include <RTS/Components/UnitComponent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>

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

void Level::CreateRandomLevel()
{
  m_GameGrid.CreateGrid(100, 100);
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

  for (ezUInt32 i = 0; i < 2; ++i)
    CreateUnit(UnitType::Default, ezVec3(i * 2.0f, 1, i * 3.0f));
}
