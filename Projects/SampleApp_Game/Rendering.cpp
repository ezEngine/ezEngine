#include "Application.h"
#include "ShipComponent.h"
#include "ProjectileComponent.h"
#include <gl/GL.h>

const float g_fShipColors[4][3] =
{
  { 1, 0, 0 },
  { 0, 1, 0 },
  { 1, 0, 1 },
  { 1, 1, 0 },
};

void RenderShip(ezGameObject* pShip, ezInt32 iPlayer)
{
  ShipComponent* pShipComponent = pShip->GetComponentOfType<ShipComponent>();

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
    glColor3f(g_fShipColors[iPlayer][0] * fHealthPerc, g_fShipColors[iPlayer][1] * fHealthPerc, g_fShipColors[iPlayer][2] * fHealthPerc);

    for (ezInt32 i = 0; i < 3; ++i)
      glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);
  glEnd();

  const float fOutline = (pShipComponent->m_iHealth <= 0) ? 0.3f : 1.0f;

  glBegin(GL_LINE_LOOP);

    glColor3f(g_fShipColors[iPlayer][0] * fOutline, g_fShipColors[iPlayer][1] * fOutline, g_fShipColors[iPlayer][2] * fOutline);

    for (ezInt32 i = 0; i < 3; ++i)
      glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);

  glEnd();
}

void RenderProjectile(ezGameObject* pObj, ProjectileComponent* pProjectile)
{
  const ezInt32 iPlayer = pProjectile->m_iBelongsToPlayer;

  ezVec3 v[2];
  v[0] = pObj->GetLocalPosition();
  v[1] = v[0] + pProjectile->m_vVelocity;

  glBegin(GL_LINES);
    glColor3f(g_fShipColors[iPlayer][0], g_fShipColors[iPlayer][1], g_fShipColors[iPlayer][2]);

    glVertex3f(v[0].x, v[0].y, v[0].z);
    glVertex3f(v[1].x, v[1].y, v[1].z);
  glEnd();
}

void SampleGameApp::RenderSingleFrame()
{
  UpdateInput();

  glViewport(0, 0, m_uiResolutionX, m_uiResolutionY);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-20, 20, -20, 20, -10, 10);


  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_CULL_FACE);

  glClearColor(0, 0, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_pLevel->Update();

  for (ezInt32 i = 0; i < MaxPlayers; ++i)
  {
    ezGameObject* pShip = m_pLevel->GetWorld()->GetObject(m_pLevel->GetPlayerShip(i));

    RenderShip(pShip, i);
  }
}


