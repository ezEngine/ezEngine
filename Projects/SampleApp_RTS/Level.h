#pragma once

#include <GameUtils/DataStructures/GameGrid.h>
#include <Core/World/World.h>
#include <SampleApp_RTS/Components/UnitComponent.h>

struct GameCellData
{
  GameCellData()
  {
    m_iCellType = 0;
    m_hUnit.Invalidate();
    m_uiVisited = 0;
  }

  static ezUInt32 s_uiVisitCounter;

  ezUInt32 m_uiVisited;
  ezInt8 m_iCellType;
  ezComponentHandle m_hUnit;
};

typedef ezGameGrid<GameCellData> GameGrid;

class Level
{
public:
  Level();
  ~Level();

  void SetupLevel();
  void Update();

  ezWorld* GetWorld() { return m_pWorld; }
  const ezWorld* GetWorld() const { return m_pWorld; }

  GameGrid& GetGrid() { return m_GameGrid; }
  const GameGrid& GetGrid() const { return m_GameGrid; }

  /// Our factory that builds all the different unit types (GameObject + components)
  ezGameObjectHandle CreateUnit(UnitType::Enum Type, const ezVec3& vPosition, const ezQuat& qRotation = ezQuat::IdentityQuaternion(), float fScaling = 1.0f);

private:
  void CreateComponentManagers();

  ezGameObject* CreateGameObject(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);
  ezGameObjectHandle CreateUnit_Default(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);

  GameGrid m_GameGrid;
  ezWorld* m_pWorld;
};

