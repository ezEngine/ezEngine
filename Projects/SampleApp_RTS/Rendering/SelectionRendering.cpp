#include <SampleApp_RTS/Rendering/Renderer.h>
#include <gl/GL.h>
#include <gl/glu.h>


void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true);

void GameRenderer::RenderSelection(const ezObjectSelection* pSelection)
{
  const ezWorld* pWorld = pSelection->GetWorld();

  for (ezUInt32 i = 0; i < pSelection->GetCount(); ++i)
  {
    ezGameObject* pUnit;
    if (pWorld->TryGetObject(pSelection->GetObject(i), pUnit))
    {
      glColor3ub(220, 50, 50);
      RenderCube(pUnit->GetLocalPosition(), ezVec3(0.6f), false);
    }
  }
}

