#include <SampleApp_RTS/Rendering/Renderer.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true);

void GameRenderer::RenderAllUnits()
{
  UnitComponentManager* pManager = m_pLevel->GetWorld()->GetComponentManager<UnitComponentManager>();

  ezBlockStorage<UnitComponent>::Iterator it = pManager->GetComponents();

  for ( ; it.IsValid(); ++it)
  {
    RenderUnit((*it).GetOwner(), &(*it));
  }
}

void GameRenderer::RenderUnit(ezGameObject* pUnit, UnitComponent* pComponent)
{
  const ezVec3& vPos = pUnit->GetLocalPosition();

  glColor3ub(50, 50, 200);

  RenderCube(vPos, ezVec3(0.5f), false);

  glColor3ub(200, 0, 200);

  if (!pComponent->m_Path.IsEmpty())
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
}


