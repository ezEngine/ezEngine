#include <PCH.h>
#include <RTS/Rendering/Renderer.h>
#include <gl/GL.h>
#include <gl/glu.h>

ezCVarBool CVarVisSteering("ai_VisSteering", false, ezCVarFlags::None, "Visualize the units collision detection whiskers and desired directions.");

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f);

void GameRenderer::RenderSelection(const ezObjectSelection* pSelection)
{
  const ezWorld* pWorld = pSelection->GetWorld();

  for (ezUInt32 i = 0; i < pSelection->GetCount(); ++i)
  {
    ezGameObject* pUnit;
    if (pWorld->TryGetObject(pSelection->GetObject(i), pUnit))
    {
      glColor3ub(220, 50, 50);
      RenderCube(pUnit->GetLocalPosition() - ezVec3(0.4f, 0, 0.4f), ezVec3(0.8f), false);
    }

    const ezVec3& vPos = pUnit->GetLocalPosition();

    UnitComponent* pComponent;

    if (!pUnit->TryGetComponentOfBaseType<UnitComponent>(pComponent))
      continue;

    if (CVarVisSteering)
    {
      ezHybridArray<SteeringBehaviorComponent*, 8> Steering;

      FollowPathSteeringComponent* pFollowSB;
      if (pUnit->TryGetComponentOfBaseType<FollowPathSteeringComponent>(pFollowSB))
        Steering.PushBack(pFollowSB);

      AvoidObstacleSteeringComponent* pAvoidSB;
      if (pUnit->TryGetComponentOfBaseType<AvoidObstacleSteeringComponent>(pAvoidSB))
        Steering.PushBack(pAvoidSB);

      float fDirectionDesire[SteeringBehaviorComponent::g_iSteeringDirections];
      float fDirectionWhisker[SteeringBehaviorComponent::g_iSteeringDirections];

      for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
      {
        fDirectionDesire[dd]  = 0.0f;
        fDirectionWhisker[dd] = 10.0f;
      }

      if (!Steering.IsEmpty())
      {

        for (ezUInt32 i = 0; i < Steering.GetCount(); ++i)
        {
          for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
          {
            fDirectionDesire[dd] = ezMath::Max(fDirectionDesire[dd], Steering[i]->GetDirectionDesire()[dd]);
            fDirectionWhisker[dd] = ezMath::Min(fDirectionWhisker[dd], Steering[i]->GetDirectionWhisker()[dd]);
          }
        }

        glBegin(GL_LINES);

        for (ezInt32 dd = 0; dd < SteeringBehaviorComponent::g_iSteeringDirections; ++dd)
        {
          fDirectionDesire[dd] = ezMath::Min(fDirectionDesire[dd], fDirectionWhisker[dd]);

          ezVec3 vW = vPos + SteeringBehaviorComponent::g_vSteeringDirections[dd] * fDirectionWhisker[dd];
          ezVec3 vD = vPos + SteeringBehaviorComponent::g_vSteeringDirections[dd] * fDirectionDesire[dd];

          glColor3ub(255, 0, 0);
          glVertex3f(vPos.x, vPos.y + 0.3f, vPos.z);
          glVertex3f(vW.x, vW.y + 0.3f, vW.z);

          glColor3ub(255, 255, 0);
          glVertex3f(vPos.x, vPos.y + 0.35f, vPos.z);
          glVertex3f(vD.x, vD.y + 0.35f, vD.z);
        }

        ezVec3 vW = vPos + SteeringBehaviorComponent::g_vSteeringDirections[pComponent->m_iCurDirection] * 10.0f;

        glColor3ub(0, 255, 0);
        glVertex3f(vPos.x, vPos.y + 0.4f, vPos.z);
        glVertex3f(vW.x, vW.y + 0.4f, vW.z);

        glEnd();
      }
    }

  }
}

