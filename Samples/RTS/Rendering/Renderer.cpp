#include <PCH.h>
#include <RTS/Rendering/Renderer.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Graphics/SimpleASCIIFont.h>
#include <gl/GL.h>
#include <gl/glu.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ColorResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ColorResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

#include <RendererGL/Device/DeviceGL.h>
void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f);

class ColorResourceLoader : public ezResourceTypeLoader
{
public:

  struct Data
  {
    Data() : m_Reader(&m_Storage) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override
  {
    ezFileReader File;

    ezStringBuilder sFile;
    sFile.Format("Textures/Streaming%i.tga", ezMath::Pow2(pResource->GetNumQualityLevelsDiscardable()));
    EZ_VERIFY(File.Open(sFile.GetData()).Succeeded(), "Failed to load image '%s'", sFile.GetData());

    Data* pData = EZ_DEFAULT_NEW(Data);

    ezMemoryStreamWriter w(&pData->m_Storage);
    w << (ezInt32) (pResource->GetNumQualityLevelsDiscardable() + 1);
    w << ezColor(1, 1, (float) pResource->GetNumQualityLevelsDiscardable() + 1.0f / (float) (pResource->GetNumQualityLevelsDiscardable() + pResource->GetNumQualityLevelsLoadable()), 0);

    while (true)
    {
      ezUInt8 uiTemp[1024];
      const ezUInt64 uiRead = File.ReadBytes(uiTemp, 1024);
      w.WriteBytes(uiTemp, uiRead);

      if (uiRead < 1024)
        break;
    }

    ezResourceLoadData LoaderData;
    LoaderData.m_pDataStream = &pData->m_Reader;
    LoaderData.m_pCustomLoaderData = pData;

    return LoaderData;
  }

  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override
  {
    Data* pData = (Data*) LoaderData.m_pCustomLoaderData;

    EZ_DEFAULT_DELETE(pData);
  }

};

ColorResourceLoader g_ColorLoader;
ColorResourceHandle g_hColorFallback;
ColorResource* g_pColorFallback;

GameRenderer::GameRenderer()
{
  m_pWindow = nullptr;
  m_pLevel = nullptr;
  m_pWorld = nullptr;
  m_pGrid = nullptr;
  m_pCamera = nullptr;
}

void GameRenderer::SetupRenderer(GameWindow* pWindow, const Level* pLevel, const ezCamera* pCamera, const ezGridNavmesh* pNavmesh)
{
  // sync these over to the renderer
  m_pWindow = pWindow;
  m_pLevel = pLevel;
  m_pWorld = m_pLevel->GetWorld();
  m_pGrid = &m_pLevel->GetGrid();
  m_pCamera = pCamera;
  m_pNavmesh = pNavmesh;
  m_uiFontTextureID = 0;
  m_fFramesPerSecond = 0;


  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = true;
  DeviceInit.m_bDebugDevice = true;
  DeviceInit.m_PrimarySwapChainDescription.m_pWindow = pWindow;
  DeviceInit.m_PrimarySwapChainDescription.m_SampleCount = ezGALMSAASampleCount::None;
  DeviceInit.m_PrimarySwapChainDescription.m_bCreateDepthStencilBuffer = true;
  DeviceInit.m_PrimarySwapChainDescription.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
  DeviceInit.m_PrimarySwapChainDescription.m_bAllowScreenshots = true;
  DeviceInit.m_PrimarySwapChainDescription.m_bVerticalSynchronization = true;

  m_pDevice = EZ_DEFAULT_NEW(ezGALDeviceGL)(DeviceInit);
  EZ_VERIFY(m_pDevice->Init() == EZ_SUCCESS, "Device init failed!");


  ezImage FontImg;
  ezGraphicsUtils::CreateSimpleASCIIFontTexture(FontImg);

  {
    ezImage FontRGBA;
    EZ_VERIFY(ezImageConversionBase::Convert(FontImg, FontRGBA, ezImageFormat::R8G8B8A8_UNORM).Succeeded(), "Could not convert image to BGRA8 format.");

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

  ezResourceManager::SetResourceTypeLoader<ColorResource>(&g_ColorLoader);

  ColorResourceDescriptor crd;
  crd.m_Color = ezColor(0, 0, 1);

  g_hColorFallback = ezResourceManager::CreateResource<ColorResource>("ColorFallback", crd);
  g_pColorFallback = ezResourceManager::BeginAcquireResource(g_hColorFallback, ezResourceAcquireMode::NoFallback);
  g_pColorFallback->m_Color = ezColor(1, 0, 0);
  ezResourceManager::EndAcquireResource(g_pColorFallback);

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
    m_fFramesPerSecond = (uiCurFrame * 1000.0) / (Now - LastFPS).GetMilliseconds();
    LastFPS = Now;
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

    RenderFormattedText(30, ALIGN_RIGHT, ezColor::White, 780, 0, "FPS: %.0f", m_fFramesPerSecond);

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

void GameRenderer::RenderConsole(ezConsole* pConsole, bool bConsoleOpen)
{
  ezMat4 mOrtho;
  mOrtho.SetOrthographicProjectionMatrix(0, 800, 800, 0, -1, 10, ezProjectionDepthRange::MinusOneToOne);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(mOrtho.m_fElementsCM);

  if (bConsoleOpen)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(10, 10, 30, 240);
    glBegin(GL_QUADS);
      glVertex3f(0, 400, -1);
      glVertex3f(800, 400, -1);
      glVertex3f(800, 0, -1);
      glVertex3f(0, 0, -1);
    glEnd();
  }

  const ezDeque<ezConsole::ConsoleString>& Strings = pConsole->GetConsoleStrings();

  const ezInt32 iTextSize = 20;
  ezInt32 iOffset = 400 - iTextSize;

  if (bConsoleOpen)
  {
    RenderFormattedText((float) iTextSize, ALIGN_LEFT, ezColor(1.0f, 0.7f, 0), 0, iOffset, ">%s", pConsole->GetInputLine());

    if (ezMath::Mod(ezTime::Now().GetMilliseconds(), 1000.0) < 500.0)
      RenderText((float) iTextSize, ALIGN_LEFT, ezColor(0,1,0), 5 + pConsole->GetCaretPosition() * (iTextSize / 2), iOffset, "|");

    iOffset -= iTextSize;
  }
  else
    iOffset = 100;

  if (bConsoleOpen)
  {
    for (ezUInt32 i = pConsole->GetScrollPosition(); i < Strings.GetCount(); ++i)
    {
      if (iOffset <= -30)
        break;

      RenderText((float) iTextSize, ALIGN_LEFT, Strings[i].m_TextColor, iTextSize / 2, iOffset, Strings[i].m_sText.GetData());

      iOffset -= iTextSize;
    }
  }
  else
  {
    const ezTime tShowMessage = ezTime::Now() - ezTime::Seconds(5.0);

    ezInt32 iShowMessages = 0;

    for (ezUInt32 i = 0; i < Strings.GetCount(); ++i)
    {
      if (iShowMessages >= 5)
        break;

      if (Strings[i].m_TimeStamp < tShowMessage)
        break;

      if (!Strings[i].m_bShowOnScreen)
        continue;

      ++iShowMessages;
      iOffset -= iTextSize;
    }

    iOffset = iShowMessages * iTextSize - iTextSize;

    for (ezUInt32 i = 0; i < Strings.GetCount(); ++i)
    {
      if (Strings[i].m_TimeStamp < tShowMessage)
        break;

      if (!Strings[i].m_bShowOnScreen)
        continue;

      RenderText((float) iTextSize, ALIGN_LEFT, Strings[i].m_TextColor, iTextSize / 2, iOffset, Strings[i].m_sText.GetData());
      iOffset -= iTextSize;
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
      ezVec2I32 iCell = Grid.GetCellAtWorldPosition(vIntersection);

      if (Grid.IsValidCellCoordinate(iCell) && Grid.GetCell(iCell).m_iCellType == 1)
      {
        glColor3ub(200, 50, 50);
        RenderCube(Grid.GetCellWorldSpaceOrigin(iCell), Grid.GetWorldSpaceCellSize(), false);
      }
    }
  }
}

void GameRenderer::Present()
{
  m_pDevice->BeginFrame();

  m_pDevice->Present(m_pDevice->GetPrimarySwapChain());

  m_pDevice->EndFrame();
}