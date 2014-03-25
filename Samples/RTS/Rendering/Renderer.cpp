#include <PCH.h>
#include <RTS/Rendering/Renderer.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Graphics/SimpleASCIIFont.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f);

GameRenderer::GameRenderer()
{
  m_pWindow = NULL;
  m_pLevel = NULL;
  m_pWorld = NULL;
  m_pGrid = NULL;
  m_pCamera = NULL;
}

void GameRenderer::SetupRenderer(const GameWindow* pWindow, const Level* pLevel, const ezCamera* pCamera, const ezGridNavmesh* pNavmesh)
{
  // sync these over to the renderer
  m_pWindow = pWindow;
  m_pLevel = pLevel;
  m_pWorld = m_pLevel->GetWorld();
  m_pGrid = &m_pLevel->GetGrid();
  m_pCamera = pCamera;
  m_pNavmesh = pNavmesh;
  m_uiFontTextureID = 0;
  m_uiFramesPerSecond = 0;

  ezImage FontImg;
  ezGraphicsUtils::CreateSimpleASCIIFontTexture(FontImg);

  {
    ezImage FontRGBA;
    EZ_VERIFY(ezImageConversionBase::Convert(FontImg, FontRGBA, ezImageFormat::R8G8B8A8_UNORM).IsSuccess(), "Could not convert image to BGRA8 format.");

    glGenTextures(1, &m_uiFontTextureID);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_uiFontTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FontRGBA.GetWidth(0), FontRGBA.GetHeight(0), 0, GL_RGBA, GL_UNSIGNED_BYTE, FontRGBA.GetPixelPointer<void>(0, 0, 0, 0, 0, 0));

    glDisable(GL_TEXTURE_2D);
  }

  UpdateState();
}

void GameRenderer::ComputeFPS()
{
  const ezTime Now = ezTime::Now();
  static ezTime LastFPS = Now;
  static ezUInt32 uiCurFrame = 0;
  
  ++uiCurFrame;

  if ((Now - LastFPS).GetSeconds() >= 1.0)
  {
    LastFPS = Now;
    m_uiFramesPerSecond = uiCurFrame;
    uiCurFrame = 0;
  }
}

void GameRenderer::RenderLevel(const ezObjectSelection* pSelection)
{
  ComputeFPS();
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

  // both of these work fine:
  m_Frustum.SetFrustum(m_pCamera->GetPosition(), m_ModelViewProjectionMatrix, m_pCamera->GetFarPlane());
  //m_Frustum.SetFrustum(m_pCamera->GetPosition(), m_pCamera->GetDirForwards(), m_pCamera->GetDirUp(), m_pCamera->GetFovX(fAspectRatio), m_pCamera->GetFovY(fAspectRatio), m_pCamera->GetFarPlane());
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
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Mouse
  {
    ezMat4 mOrtho;
    mOrtho.SetOrthographicProjectionMatrix(40, 40, -1, 10, ezProjectionDepthRange::MinusOneToOne);

    glMatrixMode(GL_PROJECTION);
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

  // Console
  {
    ezMat4 mOrtho;
    mOrtho.SetOrthographicProjectionMatrix(0, 800, 800, 0, -1, 10, ezProjectionDepthRange::MinusOneToOne);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(mOrtho.m_fElementsCM);

    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 0, "FPS: %u", m_uiFramesPerSecond);

/*
    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 40, "abcdefghijklmnopqrstuvwxyz");
    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 80, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 120, "0123456789");
    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 160, "!@#$%%^&*()_+-=");
    RenderText(40, ALIGN_LEFT, ezColor::GetWhite(), 10, 200, "\\|,<.>/?;:'\"]}[{`~");

    RenderText(20, ALIGN_LEFT, ezColor::GetWhite(), 10, 240, "abcdefghijklmnopqrstuvwxyz");
    RenderText(20, ALIGN_LEFT, ezColor::GetWhite(), 10, 260, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    RenderText(20, ALIGN_LEFT, ezColor::GetWhite(), 10, 280, "0123456789");
    RenderText(20, ALIGN_LEFT, ezColor::GetWhite(), 10, 300, "!@#$%%^&*()_+-=");
    RenderText(20, ALIGN_LEFT, ezColor::GetWhite(), 10, 320, "\\|,<.>/?;:'\"]}[{`~");

    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 370, "#include <Some/Other|blub\\File_Name.h>");
    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 400, "5-3=2, 01+6=7, 4*8!=9?");
    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 430, "[Test]: eins;\"zwem\",'dren'. 0:1;2,3.");
    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 460, "x|y|z: a^b -> {new} ~5 `X` ");

    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 490, ezStringUtf8(L"Unknown: öäüß, jippy good, font@ezEngine.net").GetData());
    RenderText(30, ALIGN_LEFT, ezColor::GetWhite(), 50, 520, "cheers, okidoki lama news");*/
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
      ezVec2I32 iCell = Grid.GetCellAtWorldPosition(vIntersection);

      if (Grid.IsValidCellCoordinate(iCell) && Grid.GetCell(iCell).m_iCellType == 1)
      {
        glColor3ub(200, 50, 50);
        RenderCube(Grid.GetCellWorldSpaceOrigin(iCell), Grid.GetWorldSpaceCellSize(), false);
      }
    }
  }
}

