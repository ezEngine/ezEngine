#include <PCH.h>
#include <SampleApp_RTS/Rendering/Renderer.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true, float fColorScale = 1.0f);

void GameRenderer::RenderGrid()
{
  const ezVec3 vCellSize = m_pGrid->GetWorldSpaceCellSize();
  const ezVec3 vCellSize2 (vCellSize.x, 0.1f, vCellSize.z);

  for (ezUInt32 z = 0; z < m_pGrid->GetGridHeight(); ++z)
  {
    for (ezUInt32 x = 0; x < m_pGrid->GetGridWidth(); ++x)
    {
      const GameCellData& cd = m_pGrid->GetCell(ezVec2I32(x, z));

      if (cd.m_uiVisibility == 0)
      {
        glColor3ub(20, 20, 20);
        RenderCube(m_pGrid->GetCellWorldSpaceOrigin(ezVec2I32(x, z)), vCellSize2, false);
        continue;
      }

      bool bColor = true;
      float fFade = 1.0f;

      if (cd.m_uiVisibility <= 150)
        fFade = (ezMath::Max<ezUInt8>(cd.m_uiVisibility, 50) / 150.0f);

      if (cd.m_iCellType == 1)
        RenderCube(m_pGrid->GetCellWorldSpaceOrigin(ezVec2I32(x, z)), vCellSize, true, fFade);
      else
        RenderCube(m_pGrid->GetCellWorldSpaceOrigin(ezVec2I32(x, z)), vCellSize2, true, fFade);
    }
  }
  
  if (false)
  {
    ezUInt8 uiGreen = 30;

    for (ezUInt32 ca = 0; ca < m_pNavmesh->GetNumConvexAreas(); ++ca)
    {
      const ezRectU32& r = m_pNavmesh->GetConvexArea(ca).m_Rect;

      const ezVec2I32 ll(r.x, r.y);
      const ezVec2I32 tr(r.x + r.width, r.y + r.height);

      const ezVec3 vLL = m_pGrid->GetCellWorldSpaceOrigin(ll);
      const ezVec3 vTR = m_pGrid->GetCellWorldSpaceOrigin(tr);

      glColor3ub(uiGreen, 20, 20);
      uiGreen += 30;
      ezVec3 vHalfWidth = (vTR - vLL);
      vHalfWidth.y = 0.5f;
      RenderCube(vLL + ezVec3(0.1f, 0, 0.1f), vHalfWidth - ezVec3(0.2f, 0, 0.2f), false);

    }
  }

  if (false)
  {
    ezUInt8 uiGreen = 30;

    for (ezUInt32 ca = 0; ca < m_pNavmesh->GetNumAreaEdges(); ++ca)
    {
      const ezGridNavmesh::AreaEdge& r = m_pNavmesh->GetAreaEdge(ca);

      const ezVec2I32 ll(r.m_EdgeRect.x, r.m_EdgeRect.y);
      const ezVec2I32 tr(r.m_EdgeRect.x + r.m_EdgeRect.width, r.m_EdgeRect.y + r.m_EdgeRect.height);

      const ezVec3 vLL = m_pGrid->GetCellWorldSpaceOrigin(ll);
      const ezVec3 vTR = m_pGrid->GetCellWorldSpaceOrigin(tr);

      glColor3ub(20, 20, uiGreen);
      uiGreen += 30;
      ezVec3 vHalfWidth = (vTR - vLL);
      vHalfWidth.y = 0.5f;
      RenderCube(vLL + ezVec3(0.1f, 0, 0.1f), vHalfWidth - ezVec3(0.2f, 0, 0.2f), false);

    }
  }
}

