#include "Application.h"
#include "Window.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include "AsteroidComponent.h"
#include <gl/GL.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Math/Color.h>

const ezColor g_fShipColors[4] =
{
  ezColor::GetRed(),
  ezColor::GetGreen(),
  ezColor::GetPink(),
  ezColor::GetYellow()
};

static void RenderShip(ezGameObject* pShip, ezInt32 iPlayer)
{
  ShipComponent* pShipComponent = NULL;
  if (!pShip->TryGetComponentOfType(pShipComponent))
    return;

  const ezVec3 vPos = pShip->GetLocalPosition();
  const ezQuat qRot = pShip->GetLocalRotation();

  ezVec3 vCorners[3];
  vCorners[0].Set(0, 0.5f, 0);
  vCorners[1].Set(- 0.2f, -0.5f, 0);
  vCorners[2].Set(+ 0.2f, -0.5f, 0);

  for (ezInt32 i = 0; i < 3; ++i)
    vCorners[i] = (qRot * vCorners[i]) + vPos;

  const float fHealthPerc = pShipComponent->m_iHealth / 1000.0f;

  glBegin(GL_TRIANGLES);
    ezColor shipColorScaled_health = g_fShipColors[iPlayer] * fHealthPerc;
    glColor4fv(shipColorScaled_health);

    for (ezInt32 i = 0; i < 3; ++i)
      glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);
  glEnd();

  const float fOutline = (pShipComponent->m_iHealth <= 0) ? 0.3f : 1.0f;

  glBegin(GL_LINE_LOOP);

    ezColor shipColorScaled_outline = g_fShipColors[iPlayer] * fOutline;
    glColor4fv(shipColorScaled_outline);

    for (ezInt32 i = 0; i < 3; ++i)
      glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);

  glEnd();
}

void SampleGameApp::RenderPlayerShips()
{
  ShipComponentManager* pShipManager = m_pLevel->GetWorld()->GetComponentManager<ShipComponentManager>();

  ezBlockStorage<ShipComponent>::Iterator it = pShipManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    RenderShip ((*it).GetOwner(), (*it).m_iPlayerIndex);
  }
}

void RenderAsteroid(ezGameObject* pObject, AsteroidComponent* pComponent)
{
  const ezVec3 vPos = pObject->GetLocalPosition();
  const ezQuat qRot = pObject->GetLocalRotation();

  glColor3ub(128, 128, 128);

  const ezInt32 iCorners = 14;
  const float fStep = 360.0f / iCorners;
  ezVec3 v[iCorners];

  for (ezInt32 i = 0; i < iCorners; ++i)
  {
    v[i].x = ezMath::Cos(ezAngle::Degree(fStep * i)) * pComponent->m_fRadius;
    v[i].y = ezMath::Sin(ezAngle::Degree(fStep * i)) * pComponent->m_fRadius;
    v[i].z = 0.0f;
  }

  srand(pComponent->m_iShapeRandomSeed);

  for (ezInt32 i = 0; i < iCorners; ++i)
    v[i] *= 0.9f + ((rand() % 1000) / 999.0f) * 0.2f;

  for (ezInt32 i = 0; i < iCorners; ++i)
    v[i] = qRot * v[i] + vPos;

  glBegin(GL_TRIANGLES);

    for (ezInt32 i = 0; i < iCorners - 2; ++i)
    {
      glVertex3f(v[0    ].x, v[0    ].y, v[0    ].z);
      glVertex3f(v[i + 1].x, v[i + 1].y, v[i + 1].z);
      glVertex3f(v[i + 2].x, v[i + 2].y, v[i + 2].z);
    }

  glEnd();
}

void SampleGameApp::RenderAsteroids()
{
  AsteroidComponentManager* pManager = m_pLevel->GetWorld()->GetComponentManager<AsteroidComponentManager>();

  ezBlockStorage<AsteroidComponent>::Iterator it = pManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    RenderAsteroid((*it).GetOwner(), &(*it));
  }
}


static void RenderProjectile(ezGameObject* pObj, ProjectileComponent* pProjectile)
{
  if (pProjectile->m_iTimeToLive <= 0)
    return;

  const ezInt32 iPlayer = pProjectile->m_iBelongsToPlayer;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  ezVec3 v[2];
  v[0] = pObj->GetLocalPosition();
  v[1] = v[0] + pProjectile->m_vDrawDir;

  ezColor projectileColor(g_fShipColors[iPlayer]);
  projectileColor.a = 1.0f - ezMath::Square(1.0f - ezMath::Clamp(pProjectile->m_iTimeToLive / 25.0f, 0.0f, 1.0f));

  glBegin(GL_LINES);
    glColor4fv(projectileColor);

    glVertex3f(v[0].x, v[0].y, v[0].z);
    glVertex3f(v[1].x, v[1].y, v[1].z);
  glEnd();

  glDisable(GL_BLEND);
}

void SampleGameApp::RenderProjectiles()
{
  ProjectileComponentManager* pProjectileManager = m_pLevel->GetWorld()->GetComponentManager<ProjectileComponentManager>();

  ezBlockStorage<ProjectileComponent>::Iterator it = pProjectileManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    RenderProjectile ((*it).GetOwner(), &(*it));
  }
}

ezCVarInt CVarGameSlowDown("CVar_GameSlowDown", 0, ezCVarFlags::Default, "How much to slow down the game (20 is a good value).");

void SampleGameApp::RenderSingleFrame()
{
  // always update the game with a fixed time step of 1/60 seconds
  const ezTime tUpdateInterval = ezTime::Seconds(1.0 / 60.0);
  const ezTime tNow = ezSystemTime::Now();

  static ezTime s_LastGameUpdate = tNow;

  // This loop lets you artificially slow down the game to see how the fixed
  // game time stepping kicks in to do more updates per frame and keep a stable simulation
  //if (false)
  {
    // enable this loop to reduce the overall framerate
    for (int i = 0; i < CVarGameSlowDown; ++i)
    {
      ezThreadUtils::Sleep(1);
      ezInputManager::PollHardware();
    }
  }

  while (tNow - s_LastGameUpdate > tUpdateInterval)
  {
    UpdateInput(tUpdateInterval);
    m_pLevel->Update();

    s_LastGameUpdate += tUpdateInterval;
  }

  ezSizeU32 resolution = m_pWindow->GetResolution();
  glViewport(0, 0, resolution.width, resolution.height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-20, 20, -20, 20, -10, 10);


  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_CULL_FACE);

  glClearColor(0, 0, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  RenderProjectiles();
  RenderAsteroids();
  RenderPlayerShips();

  if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint0) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint0_PositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint0_PositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(255, 255, 255);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint1) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint1_PositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint1_PositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(255, 255, 255);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint2) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint2_PositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint2_PositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(255, 255, 255);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint3) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint3_PositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint3_PositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(255, 0, 0);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint4) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint4_PositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint4_PositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(255, 0, 0);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  //if (ezInputManager::GetInputSlotState(ezInputSlot_TouchPoint0) == ezKeyState::Down)
  {
    float pX = 0, pY = 0;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &pX);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &pY);

  
    glBegin(GL_POINTS);
      glColor3ub(0, 255, 0);

      glVertex3f(pX * 40.0f - 20.0f, pY * -40.0f + 20.0f, 0);

    glEnd();
  }

  if (m_pThumbstick && m_pThumbstick->IsEnabled())
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3ub(0, 0, 255);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, 1.0f, 1.0f, 0.0f, -10, 10);

    ezVec2 vLL, vUR;
    m_pThumbstick->GetInputArea(vLL, vUR);

    glBegin(GL_QUADS);
      glVertex3f(vLL.x, vLL.y, 0.0f);
      glVertex3f(vUR.x, vLL.y, 0.0f);
      glVertex3f(vUR.x, vUR.y, 0.0f);
      glVertex3f(vLL.x, vUR.y, 0.0f);
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if (m_pThumbstick2 && m_pThumbstick2->IsEnabled())
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3ub(0, 0, 255);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, 1.0f, 1.0f, 0.0f, -10, 10);

    ezVec2 vLL, vUR;
    m_pThumbstick2->GetInputArea(vLL, vUR);

    glBegin(GL_QUADS);
      glVertex3f(vLL.x, vLL.y, 0.0f);
      glVertex3f(vUR.x, vLL.y, 0.0f);
      glVertex3f(vUR.x, vUR.y, 0.0f);
      glVertex3f(vLL.x, vUR.y, 0.0f);
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}


