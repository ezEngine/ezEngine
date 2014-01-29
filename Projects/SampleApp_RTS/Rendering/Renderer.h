#pragma once

#include <CoreUtils/Graphics/Camera.h>
#include <SampleApp_RTS/Level.h>
#include <GameUtils/DataStructures/GameGrid.h>
#include <SampleApp_RTS/General/Window.h>
#include <GameUtils/DataStructures/ObjectSelection.h>

class GameRenderer
{
public:
  GameRenderer();

  void SetupRenderer(const GameWindow* pWindow, const Level* pLevel, const ezCamera* pCamera, const ezGridNavmesh* pNavmesh);

  void RenderLevel(const ezObjectSelection* pSelection);

  bool GetPickingRay(float fMousePosX, float fMousePosY, ezVec3& out_RayPos, ezVec3& out_RayDir);

private:
  void UpdateState();

  const GameWindow* m_pWindow;
  const Level* m_pLevel;
  const ezWorld* m_pWorld;
  const GameGrid* m_pGrid;
  const ezCamera* m_pCamera;
  const ezGridNavmesh* m_pNavmesh;

  ezMat4 m_ProjectionMatrix;
  ezMat4 m_ModelViewMatrix;
  ezMat4 m_ModelViewProjectionMatrix;
  ezMat4 m_InverseProjectionMatrix;
  ezMat4 m_InverseModelViewMatrix;
  ezMat4 m_InverseModelViewProjectionMatrix;

  void Render3D(const ezObjectSelection* pSelection);
  void Render2DOverlays();

  void RenderAllUnits();
  void RenderUnit(ezGameObject* pUnit, UnitComponent* pComponent);

  void RenderSelection(const ezObjectSelection* pSelection);

  void RenderGrid();
  void RenderMousePicking();

};

