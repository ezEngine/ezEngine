#include <PCH.h>
#include <RTS/Rendering/Renderer.h>
#include <RTS/Level.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f);
void RenderCircleXZ(const ezVec3& v, float fRadius, const ezMat4& mRot = ezMat4::IdentityMatrix());


ezCVarBool CVarVisObstacles("ai_VisObstacles", true, ezCVarFlags::None, "Visualize obstacle radii.");
ezCVarBool CVarVisPath("ai_VisPath", false, ezCVarFlags::None, "Visualize the units path.");

void GameRenderer::RenderAllUnits()
{
  UnitComponentManager* pManager = m_pLevel->GetWorld()->GetComponentManager<UnitComponentManager>();

  for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
  {
    RenderUnit(it->GetOwner(), it);
  }
}

void GameRenderer::RenderUnit(ezGameObject* pUnit, UnitComponent* pComponent)
{
  const ezVec3& vPos = pUnit->GetLocalPosition();

  glColor3ub(50, 50, 200);

  RenderCube(pUnit->GetLocalPosition() - ezVec3(0.3f, 0, 0.3f), ezVec3(0.6f), false);

  glColor3ub(200, 0, 200);

  if (!pComponent->m_Path.IsEmpty() && CVarVisPath)
  {
    glBegin(GL_LINE_STRIP);
    glVertex3f(vPos.x, vPos.y, vPos.z);

    for (ezUInt32 i = 0; i < pComponent->m_Path.GetCount(); ++i)
    {
      ezVec3 v = pComponent->m_Path[i];
      glVertex3f(v.x, v.y, v.z);
    }
    
    glEnd();
  }



  if (CVarVisObstacles)
  {
    ezHybridArray<ObstacleComponent*, 8> Components;
    pUnit->TryGetComponentsOfBaseType<ObstacleComponent>(Components);

    ezVec3 vCenter = pUnit ->GetLocalPosition();
    vCenter.y -= 0.7f;

    glColor3ub(0, 200, 0);
    for (ezUInt32 c = 0; c < Components.GetCount(); ++c)
    {
      RenderCircleXZ(vCenter, Components[c]->m_fRadius > 0 ? Components[c]->m_fRadius : ObstacleComponent::g_fDefaultRadius);
    }
  }
}



