#pragma once

#include <GameUtils/DataStructures/GameGrid.h>
#include <GameUtils/PathFinding/GridNavmesh.h>
#include <Core/World/World.h>
#include <SampleApp_RTS/Components/UnitComponent.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

struct GameCellData
{
  GameCellData()
  {
    m_iCellType = 0;
    m_uiVisibility = 0;
  }

  ezUInt8 m_uiVisibility;
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

  ezGridNavmesh& GetNavmesh() { return m_Navmesh; }
  const ezGridNavmesh& GetNavmesh() const { return m_Navmesh; }

  /// Our factory that builds all the different unit types (GameObject + components)
  ezGameObjectHandle CreateUnit(UnitType::Enum Type, const ezVec3& vPosition, const ezQuat& qRotation = ezQuat::IdentityQuaternion(), float fScaling = 1.0f);

private:
  void CreateComponentManagers();
  void CreateRandomLevel();

  static bool IsSameCellType(ezUInt32 uiCell1, ezUInt32 uiCell2, void* pPassThrough);
  static bool IsCellBlocked(ezUInt32 uiCell, void* pPassThrough);
  static ezCallbackResult::Enum SetPointBlocking(ezInt32 x, ezInt32 y, void* pPassThrough);

  ezGameObject* CreateGameObject(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);
  ezGameObjectHandle CreateUnit_Default(const ezVec3& vPosition, const ezQuat& qRotation, float fScaling);

  GameGrid m_GameGrid;
  ezGridNavmesh m_Navmesh;
  ezWorld* m_pWorld;
};

