#include <SampleApp_RTS/Rendering/Renderer.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true);

GameRenderer::GameRenderer()
{
  m_pWindow = NULL;
  m_pLevel = NULL;
  m_pWorld = NULL;
  m_pGrid = NULL;
  m_pCamera = NULL;
}

void GameRenderer::SetupRenderer(const GameWindow* pWindow, const Level* pLevel, const ezCamera* pCamera)
{
  // sync these over to the renderer
  m_pWindow = pWindow;
  m_pLevel = pLevel;
  m_pWorld = m_pLevel->GetWorld();
  m_pGrid = &m_pLevel->GetGrid();
  m_pCamera = pCamera;

  UpdateState();
}

void GameRenderer::RenderLevel(const ezObjectSelection* pSelection)
{
  UpdateState();
  Render3D(pSelection);
  Render2DOverlays();
}

void GameRenderer::UpdateState()
{
  const ezSizeU32 Resolution = m_pWindow->GetResolution();
  const float fAspectRatio = (float) Resolution.width / (float) Resolution.height;

  m_ProjectionMatrix.SetPerspectiveProjectionMatrixFromFovY(m_pCamera->GetFovY(fAspectRatio), fAspectRatio, m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane(), ezProjectionDepthRange::MinusOneToOne);

  m_ModelViewMatrix.SetLookAtMatrix(m_pCamera->GetPosition(), m_pCamera->GetPosition() + m_pCamera->GetDirForwards(), m_pCamera->GetDirUp());

  m_ModelViewProjectionMatrix = m_ProjectionMatrix * m_ModelViewMatrix;

  m_InverseProjectionMatrix = m_ProjectionMatrix.GetInverse();
  m_InverseModelViewMatrix = m_ModelViewMatrix.GetInverse();
  m_InverseModelViewProjectionMatrix = m_ModelViewProjectionMatrix.GetInverse();
}

void GameRenderer::Render3D(const ezObjectSelection* pSelection)
{
  const ezSizeU32 Resolution = m_pWindow->GetResolution();

  glViewport(0, 0, Resolution.width, Resolution.height);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m_ProjectionMatrix.m_fElementsCM);

  glLoadMatrixf(m_ModelViewProjectionMatrix.m_fElementsCM);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_CULL_FACE);

  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0f);

  glClearColor(0, 0, 0.2f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  RenderMousePicking();
  RenderGrid();
  RenderAllUnits();
  RenderSelection(pSelection);
}

void GameRenderer::Render2DOverlays()
{
  // Mouse
  {
    ezMat4 mOrtho;
    mOrtho.SetOrthographicProjectionMatrix(40, 40, -1, 10, ezProjectionDepthRange::MinusOneToOne);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-20, 20, -20, 20, -1, 10);

    float fOrtho[16];
    glGetFloatv(GL_PROJECTION_MATRIX, fOrtho);

    glLoadMatrixf(mOrtho.m_fElementsCM);

    {
      float pX = 0, pY = 0;
      ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &pX);
      ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &pY);

  
      glBegin(GL_POINTS);
        glColor3ub(0, 255, 0);

        glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

      glEnd();
    }
  }
}

void GameRenderer::RenderMousePicking()
{
  float fMouseX, fMouseY;
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &fMouseX);
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &fMouseY);

  ezVec3 vWorldPos;
  ezVec3 vDirToWorldPos;

  if (GetPickingRay(fMouseX, fMouseY, vWorldPos, vDirToWorldPos))
  {
    ezPlane Ground;
    Ground.SetFromNormalAndPoint(ezVec3(0, 1, 0), ezVec3(0));

    float fIntersection;
    ezVec3 vIntersection;
    if (Ground.GetRayIntersection(m_pCamera->GetPosition(), vDirToWorldPos, &fIntersection, &vIntersection))
    {
      glColor3ub(255, 0, 0);
      glBegin(GL_LINES);
        glVertex3f(vIntersection.x, vIntersection.y, vIntersection.z);
        glVertex3f(vIntersection.x, vIntersection.y + 5, vIntersection.z);
      glEnd();

      const GameGrid& Grid = m_pLevel->GetGrid();
      ezGridCoordinate iCell = Grid.GetCellAtPosition(vIntersection);

      if (Grid.IsValidCellCoordinate(iCell) && Grid.GetCell(iCell).m_iCellType == 1)
      {
        glColor3ub(200, 50, 50);
        RenderCube(Grid.GetCellOrigin(iCell), Grid.GetCellSize(), false);
      }
    }
  }
}

