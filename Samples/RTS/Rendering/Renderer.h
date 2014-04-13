#pragma once

#include <CoreUtils/Graphics/Camera.h>
#include <CoreUtils/Console/Console.h>
#include <RTS/Level.h>
#include <GameUtils/DataStructures/GameGrid.h>
#include <RTS/General/Window.h>
#include <GameUtils/DataStructures/ObjectSelection.h>
#include <Foundation/Math/Frustum.h>
#include <Core/ResourceManager/ResourceManager.h>

class ezGALDeviceGL;

class GameRenderer
{
public:
  GameRenderer();

  void SetupRenderer(GameWindow* pWindow, const Level* pLevel, const ezCamera* pCamera, const ezGridNavmesh* pNavmesh);

  void RenderLevel(const ezObjectSelection* pSelection);

  bool GetPickingRay(float fMousePosX, float fMousePosY, ezVec3& out_RayPos, ezVec3& out_RayDir);

  void RenderConsole(ezConsole* pConsole, bool bConsoleOpen);

  void Present();

private:
  void UpdateState();

  const GameWindow* m_pWindow;
  const Level* m_pLevel;
  const ezWorld* m_pWorld;
  const GameGrid* m_pGrid;
  const ezCamera* m_pCamera;
  const ezGridNavmesh* m_pNavmesh;

  ezGALDeviceGL* m_pDevice;

  ezMat4 m_ProjectionMatrix;
  ezMat4 m_ModelViewMatrix;
  ezMat4 m_ModelViewProjectionMatrix;
  ezMat4 m_InverseProjectionMatrix;
  ezMat4 m_InverseModelViewMatrix;
  ezMat4 m_InverseModelViewProjectionMatrix;
  ezFrustum m_Frustum;

  void Render3D(const ezObjectSelection* pSelection);
  void Render2DOverlays();

  void RenderAllUnits();
  void RenderUnit(ezGameObject* pUnit, UnitComponent* pComponent);

  void RenderSelection(const ezObjectSelection* pSelection);

  void RenderGrid();
  void RenderMousePicking();

  void ComputeFPS();

  enum TextAlignment
  {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
  };

  void RenderFormattedText(float fTextSize, TextAlignment Align, ezColor Color, ezInt32 x, ezInt32 y, const char* szText, ...);

  void RenderText(float fTextSize, TextAlignment Align, ezColor Color, ezInt32 x, ezInt32 y, const char* szText);

  ezUInt32 m_uiFontTextureID;

  double m_fFramesPerSecond;
};

