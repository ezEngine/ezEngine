#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>

#include <Core/Application/Application.h>
#include <Core/World/World.h>

#include <Core/Input/InputManager.h>
#include <InputXBox360/InputDeviceXBox.h>

#include "DamageComponent.h"
#include "HealthComponent.h"

#define MaxPlayers 4

void CreateAppWindow();
void DestroyAppWindow();
void GameLoop();
void SetupInput();

ezGameObjectHandle m_ShipPlayer[MaxPlayers];
ezWorld* m_pWorld;

const char* szControls[4][5] = 
{ 
  { "Player1_Forwards", "Player1_Backwards", "Player1_RotateLeft", "Player1_RotateRight", "Player1_Shooting" },
  { "Player2_Forwards", "Player2_Backwards", "Player2_RotateLeft", "Player2_RotateRight", "Player2_Shooting" },
  { "Player3_Forwards", "Player3_Backwards", "Player3_RotateLeft", "Player3_RotateRight", "Player3_Shooting" },
  { "Player4_Forwards", "Player4_Backwards", "Player4_RotateLeft", "Player4_RotateRight", "Player4_Shooting" } 
};

class SampleGameApp : public ezApplication
{
public:


  virtual void AfterEngineInit() EZ_OVERRIDE
  {
    ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    m_pWorld = EZ_DEFAULT_NEW(ezWorld)("Test");

    //ezGameObjectHandle handle;
    //ezGameObject* pObject;

    //{
    //  ezGameObjectDesc desc;
    //  handle = m_pWorld->CreateObject(desc);
    //  pObject = m_pWorld->GetObject(handle);
    //}

    //{
    //  HealthComponentManager* pManager = m_pWorld->CreateComponentManager<HealthComponentManager>();

    //  ezComponentHandle healthComponent = pManager->CreateComponent();

    //  HealthComponent* pHealthComponent = pManager->GetComponent(healthComponent);
    //  pHealthComponent->SetHealth(80.0f);

    //  pObject->AddComponent(healthComponent);
    //}

    //{
    //  pObject->AddComponent(m_pWorld->CreateComponentManager<DamageComponentManager>()->CreateComponent());
    //}

    SetupInput();

    DamageComponentManager* pManager = m_pWorld->CreateComponentManager<DamageComponentManager>();
    HealthComponentManager* pManagerProjectile = m_pWorld->CreateComponentManager<HealthComponentManager>();

    for (ezInt32 i = 0; i < MaxPlayers; ++i)
    {
      ezGameObjectDesc desc;
      desc.m_LocalPosition.x = -15 + i * 5.0f;
      m_ShipPlayer[i] = m_pWorld->CreateObject(desc);

      ezGameObject* pShip = m_pWorld->GetObject(m_ShipPlayer[i]);

      pShip->AddComponent(pManager->CreateComponent());
      pShip->GetComponentOfType<DamageComponent>()->m_iPlayerIndex = i;
    }
  }

  virtual void BeforeEngineShutdown() EZ_OVERRIDE
  {
    EZ_DEFAULT_DELETE(m_pWorld);
  }

  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE
  {
    //m_pWorld->Update();

    //if (m_pWorld->GetObjectCount() > 0)
    //  return ezApplication::Continue;

    CreateAppWindow();
    GameLoop();
    DestroyAppWindow();

    return ezApplication::Quit;
  }

private:

};

EZ_CONSOLEAPP_ENTRY_POINT(SampleGameApp);

#include <gl/GL.h>

extern bool g_bActiveRenderLoop;

void SetupInput()
{
  g_InputDeviceXBox360.EnableVibration(0, true);

  {
    ezInputManager::ezInputActionConfig cfg;

    cfg = ezInputManager::ezInputActionConfig();
    cfg.m_sInputSlotTrigger[0] = "keyboard_escape";
    ezInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);
  }

  // ** Player 1 **

  ezInt32 iPlayer = 0;

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_up";
    cfg.m_sInputSlotTrigger[1] = "controller0_leftstick_posy";
    cfg.m_sInputSlotTrigger[2] = "controller0_pad_up";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][0], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_down";
    cfg.m_sInputSlotTrigger[1] = "controller0_leftstick_negy";
    cfg.m_sInputSlotTrigger[2] = "controller0_pad_down";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][1], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_left";
    cfg.m_sInputSlotTrigger[1] = "controller0_rightstick_negx";
    cfg.m_sInputSlotTrigger[2] = "controller0_pad_left";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][2], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_right";
    cfg.m_sInputSlotTrigger[1] = "controller0_rightstick_posx";
    cfg.m_sInputSlotTrigger[2] = "controller0_pad_right";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][3], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_right_ctrl";
    cfg.m_sInputSlotTrigger[1] = "controller0_right_trigger";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][4], cfg, true);
  }

  // ** Player 2 **
  iPlayer = 1;

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_w";
    cfg.m_sInputSlotTrigger[1] = "controller1_leftstick_posy";
    cfg.m_sInputSlotTrigger[2] = "controller1_pad_up";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][0], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_s";
    cfg.m_sInputSlotTrigger[1] = "controller1_leftstick_negy";
    cfg.m_sInputSlotTrigger[2] = "controller1_pad_down";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][1], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_a";
    cfg.m_sInputSlotTrigger[1] = "controller1_rightstick_negx";
    cfg.m_sInputSlotTrigger[2] = "controller1_pad_left";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][2], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_d";
    cfg.m_sInputSlotTrigger[1] = "controller1_rightstick_posx";
    cfg.m_sInputSlotTrigger[2] = "controller1_pad_right";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][3], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "keyboard_left_ctrl";
    cfg.m_sInputSlotTrigger[1] = "controller1_right_trigger";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][4], cfg, true);
  }

  // ** Player 3 **
  iPlayer = 2;

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller2_leftstick_posy";
    cfg.m_sInputSlotTrigger[1] = "controller2_pad_up";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][0], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller2_leftstick_negy";
    cfg.m_sInputSlotTrigger[1] = "controller2_pad_down";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][1], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller2_rightstick_negx";
    cfg.m_sInputSlotTrigger[1] = "controller2_pad_left";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][2], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller2_rightstick_posx";
    cfg.m_sInputSlotTrigger[1] = "controller2_pad_right";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][3], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[1] = "controller2_right_trigger";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][4], cfg, true);
  }

  // ** Player 4 **
  iPlayer = 3;

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller3_leftstick_posy";
    cfg.m_sInputSlotTrigger[1] = "controller3_pad_up";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][0], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller3_leftstick_negy";
    cfg.m_sInputSlotTrigger[1] = "controller3_pad_down";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][1], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller3_rightstick_negx";
    cfg.m_sInputSlotTrigger[1] = "controller3_pad_left";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][2], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[0] = "controller3_rightstick_posx";
    cfg.m_sInputSlotTrigger[1] = "controller3_pad_right";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][3], cfg, true);
  }

  {
    ezInputManager::ezInputActionConfig cfg;
    cfg.m_sInputSlotTrigger[1] = "controller3_right_trigger";

    ezInputManager::SetInputActionConfig("Game", szControls[iPlayer][4], cfg, true);
  }
}

void UpdateShipPlayer(int iPlayer)
{
  float fVal = 0.0f;

  ezGameObject* pShip = m_pWorld->GetObject(m_ShipPlayer[iPlayer]);
  DamageComponent* pShipComponent = pShip->GetComponentOfType<DamageComponent>();

  ezVec3 vVelocity(0.0f);


  const ezQuat qRot = pShip->GetLocalRotation();
  const ezVec3 vShipDir = qRot * ezVec3(0, 1, 0);


  if (ezInputManager::GetInputActionState("Game", szControls[iPlayer][0], &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    vVelocity += 0.1f * vShipDir * fVal;

    pShip->SetLocalPosition(vPos);
  }

  if (ezInputManager::GetInputActionState("Game", szControls[iPlayer][1], &fVal) != ezKeyState::Up)
  {
    ezVec3 vPos = pShip->GetLocalPosition();
    vVelocity -= 0.1f * vShipDir * fVal;

    pShip->SetLocalPosition(vPos);
  }

  if (!vVelocity.IsZero())
    pShipComponent->SetVelocity(vVelocity);

  if (ezInputManager::GetInputActionState("Game", szControls[iPlayer][2], &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), 4.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", szControls[iPlayer][3], &fVal) != ezKeyState::Up)
  {
    ezQuat qRotation;
    qRotation.SetFromAxisAndAngle(ezVec3(0, 0, 1), -4.0f * fVal);

    ezQuat qNewRot = qRotation * pShip->GetLocalRotation();
    pShip->SetLocalRotation(qNewRot);
  }

  if (ezInputManager::GetInputActionState("Game", szControls[iPlayer][4], &fVal) != ezKeyState::Up)
  {
    pShipComponent->SetIsShooting(true);
  }
  else
    pShipComponent->SetIsShooting(false);
}

void RenderShip(ezGameObject* pShip, ezInt32 iPlayer)
{
  DamageComponent* pShipComponent = pShip->GetComponentOfType<DamageComponent>();

  float fHealthPerc = pShipComponent->m_iHealth / 1000.0f;

  const ezVec3 vPos = pShip->GetLocalPosition();
  const ezQuat qRot = pShip->GetLocalRotation();

  ezVec3 vCorners[3];
  vCorners[0].Set(0, 0.5f, 0);
  vCorners[1].Set(- 0.2f, -0.5f, 0);
  vCorners[2].Set(+ 0.2f, -0.5f, 0);

  for (ezInt32 i = 0; i < 3; ++i)
    vCorners[i] = (qRot * vCorners[i]) + vPos;

  glBegin(GL_TRIANGLES);
  switch(iPlayer)
  {
  case 0:
    glColor3f(fHealthPerc, 0, 0);
    break;
  case 1:
    glColor3f(0, fHealthPerc, 0);
    break;
  case 2:
    glColor3f(0, 0, fHealthPerc);
    break;
  case 3:
    glColor3f(fHealthPerc, fHealthPerc, 0);
    break;
  };


  for (ezInt32 i = 0; i < 3; ++i)
    glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);
  glEnd();

  float fOutline = 1.0f;

  if (pShipComponent->m_iHealth <= 0)
    fOutline = 0.3f;

  glBegin(GL_LINE_LOOP);
  switch(iPlayer)
  {
  case 0:
    glColor3f(fOutline, 0, 0);
    break;
  case 1:
    glColor3f(0, fOutline, 0);
    break;
  case 2:
    glColor3f(0, 0, fOutline);
    break;
  case 3:
    glColor3f(fOutline, fOutline, 0);
    break;
  };

  for (ezInt32 i = 0; i < 3; ++i)
    glVertex3f(vCorners[i].x, vCorners[i].y, vCorners[i].z);
  glEnd();
}

void RenderProjectile(ezGameObject* pObj, HealthComponent* pProjectile)
{
  ezVec3 v[2];
  v[0] = pObj->GetLocalPosition();
  v[1] = v[0] + pProjectile->m_vVelocity * 1.0f;

  glColor3ub(255, 100, 0);

  switch(pProjectile->m_iBelongsToPlayer)
  {
  case 0:
    glColor3ub(150, 0, 0);
    break;
  case 1:
    glColor3ub(0, 150, 0);
    break;
  case 2:
    glColor3ub(0, 0, 150);
    break;
  case 3:
    glColor3ub(150, 150, 0);
    break;
  }

  glBegin(GL_LINES);


  glVertex3f(v[0].x, v[0].y, v[0].z);
  glVertex3f(v[1].x, v[1].y, v[1].z);
  glEnd();
}

void RenderSingleFrame()
{
  ezInputManager::Update();

  if (ezInputManager::GetInputActionState("Main", "CloseApp") == ezKeyState::Pressed)
  {
    g_bActiveRenderLoop = false;
    return;
  }

  for (ezInt32 i = 0; i < MaxPlayers; ++i)
  {
    UpdateShipPlayer(i);
  }



  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-20, 20, -20, 20, -10, 10);


  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_CULL_FACE);

  glClearColor(0, 0, 0.1f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_pWorld->Update();

  DamageComponentManager* pManagerShips = m_pWorld->CreateComponentManager<DamageComponentManager>();
  HealthComponentManager* pManagerProjectile = m_pWorld->CreateComponentManager<HealthComponentManager>();

  for (ezInt32 i = 0; i < MaxPlayers; ++i)
  {
    ezGameObject* pShip = m_pWorld->GetObject(m_ShipPlayer[i]);

    RenderShip(pShip, i);
  }

}




void HealthComponent::Update()
{
  //EZ_ASSERT(!m_bDeleted, "Bah!");

  if (m_iTimeToLive <= 0)
  {
    //m_bDeleted = true;
    //GetManager()->DeleteComponent(GetHandle());
    return;
  }

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);
  --m_iTimeToLive;

  RenderProjectile(m_pOwner, this);

  for (ezInt32 p = 0; p < MaxPlayers; ++p)
  {
    if (p == m_iBelongsToPlayer)
      continue;

    ezGameObject* pEnemy = GetWorld()->GetObject(m_ShipPlayer[p]);
    DamageComponent* pEnemeyComponent = pEnemy->GetComponentOfType<DamageComponent>();

    if (pEnemeyComponent->m_iHealth <= 0)
      continue;

    ezBoundingSphere bs(pEnemy->GetLocalPosition(), 1.5f);

    const ezVec3 vPos = m_pOwner->GetLocalPosition();

    if (!m_vVelocity.IsZero(0.001f) && bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
    {
      if (m_bDoesDamage)
      {
        pEnemeyComponent->m_iHealth = ezMath::Max(pEnemeyComponent->m_iHealth - 100, 0);

        const ezInt32 iMaxParticles = 100;

        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), 180.0f + (float) (i - (iMaxParticles / 2)) * fSteps);
            const ezVec3 vDir = qRot * m_vVelocity;

            {
              ezGameObjectDesc desc;
              desc.m_LocalPosition = m_pOwner->GetLocalPosition();
              ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

              ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);
              pProjectile->AddComponent(GetWorld()->GetComponentManager<HealthComponentManager>()->CreateComponent());

              HealthComponent* pProjectileComponent = pProjectile->GetComponentOfType<HealthComponent>();
              pProjectileComponent->m_iBelongsToPlayer = m_iBelongsToPlayer;
              pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
              pProjectileComponent->m_bDoesDamage = false;
            }
          }
        }

        {
          const float fAngle = 10.0f + (rand() % 90);
          const float fSteps = fAngle / iMaxParticles;
        
          for (ezInt32 i = 0; i < iMaxParticles; ++i)
          {
            ezQuat qRot;
            qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), 180.0f + (float) (i - (iMaxParticles / 2)) * fSteps);
            const ezVec3 vDir = qRot * m_vVelocity;

            {
              ezGameObjectDesc desc;
              desc.m_LocalPosition = m_pOwner->GetLocalPosition();
              ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

              ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);
              pProjectile->AddComponent(GetWorld()->GetComponentManager<HealthComponentManager>()->CreateComponent());

              HealthComponent* pProjectileComponent = pProjectile->GetComponentOfType<HealthComponent>();
              pProjectileComponent->m_iBelongsToPlayer = pEnemeyComponent->m_iPlayerIndex;
              pProjectileComponent->m_vVelocity = vDir * (1.0f + ((rand() % 1000) / 999.0f));
              pProjectileComponent->m_bDoesDamage = false;
            }
          }
        }
      }

      m_iTimeToLive = 0;
    }
  }
}

void DamageComponent::Update()
{
  if (m_iHealth <= 0)
    return;

  //#if 0
  //    if (HealthComponent* pHealthComponent = m_pOwner->GetComponentOfType<HealthComponent>())
  //    {
  //      float fHealth = pHealthComponent->GetHealth();
  //
  //      fHealth -= m_fDamagePerSecond * (1.0f / 60.0f); // TODO: timer class
  //
  //      pHealthComponent->SetHealth(fHealth);
  //    }
  //#else
  //    HealthMessage msg;
  //    msg.m_fHealthInc = -m_fDamagePerSecond * (1.0f / 60.0f);  // TODO: timer class
  //
  //    m_pOwner->SendMessage(msg);
  //#endif

  const ezVec3 vShipDir = m_pOwner->GetLocalRotation() * ezVec3(0, 1, 0);

  if (!m_vVelocity.IsZero(0.001f))
  {
    for (ezInt32 p = 0; p < MaxPlayers; ++p)
    {
      if (p == m_iPlayerIndex)
        continue;

      ezGameObject* pEnemy = GetWorld()->GetObject(m_ShipPlayer[p]);
      DamageComponent* pEnemeyComponent = pEnemy->GetComponentOfType<DamageComponent>();

      if (pEnemeyComponent->m_iHealth <= 0)
        continue;

      ezBoundingSphere bs(pEnemy->GetLocalPosition(), 1.0f);

      const ezVec3 vPos = m_pOwner->GetLocalPosition();

      if (bs.GetLineSegmentIntersection(vPos, vPos + m_vVelocity))
      {
        m_vVelocity *= -1;
      }
    }
  }

  m_pOwner->SetLocalPosition(m_pOwner->GetLocalPosition() + m_vVelocity);
  m_vVelocity *= 0.95f;

  if (m_iShootDelay > 0)
    --m_iShootDelay;
  else
    if (m_bIsShooting && m_iAmmunition >= m_iAmmoPerShot)
    {
      m_iShootDelay = 4;

      ezGameObjectDesc desc;
      desc.m_LocalPosition = m_pOwner->GetLocalPosition();
      ezGameObjectHandle hProjectile = GetWorld()->CreateObject(desc);

      ezGameObject* pProjectile = GetWorld()->GetObject(hProjectile);

      pProjectile->AddComponent(GetWorld()->GetComponentManager<HealthComponentManager>()->CreateComponent());

      HealthComponent* pProjectileComponent = pProjectile->GetComponentOfType<HealthComponent>();
      pProjectileComponent->m_iBelongsToPlayer = m_iPlayerIndex;
      pProjectileComponent->m_vVelocity = vShipDir * ezMath::Max(m_vVelocity.GetLength(), 1.0f) * 1.0f;
      pProjectileComponent->m_bDoesDamage = true;

      m_iAmmunition -= m_iAmmoPerShot;
    }

    m_iAmmunition = ezMath::Clamp(m_iAmmunition + 1, 0, 100);
    //m_iHealth = ezMath::Clamp(m_iHealth + 1, 0, 1000);


    ezVec3 vCurPos = m_pOwner->GetLocalPosition();
    vCurPos = vCurPos.CompMax(ezVec3(-20.0f));
    vCurPos = vCurPos.CompMin(ezVec3(20.0f));

    m_pOwner->SetLocalPosition(vCurPos);
}
