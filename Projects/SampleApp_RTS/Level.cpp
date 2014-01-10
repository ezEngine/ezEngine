#include <SampleApp_RTS/Level.h>
#include <SampleApp_RTS/General/Application.h>
#include <SampleApp_RTS/Components/UnitComponent.h>

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

void Level::SetupLevel()
{
  m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Level");
  m_pWorld->SetUserData(this);

  m_GameGrid.CreateGrid(50, 50, 1);
  m_GameGrid.SetWorldDimensions(ezVec3(-25.0f, 0, -25.0f), ezVec3(1.0f, 2.0f, 1.0f));

  for (ezUInt32 slice = 0; slice < m_GameGrid.GetSlices(); ++slice)
  {
    for (ezUInt32 z = 0; z < m_GameGrid.GetDepth(); ++z)
    {
      for (ezUInt32 x = 0; x < m_GameGrid.GetWidth(); ++x)
      {
        if (rand() % 30 == 1)
          m_GameGrid.GetCell(ezGridCoordinate(x, z, slice)).m_iCellType = 1;
      }
    }
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
    m_GameGrid.GetCellByIndex(i).m_bOccupied = false;
    //m_GameGrid.GetCellByIndex(i).m_uiVisited = 0;
  }

  m_pWorld->Update();

}


