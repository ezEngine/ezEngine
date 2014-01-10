#include <SampleApp_RTS/Rendering/Renderer.h>
#include <gl/GL.h>
#include <gl/glu.h>

void RenderCube(const ezVec3& v, const ezVec3& s, bool bColor = true);

void GameRenderer::RenderGrid()
{
  const ezVec3 vCellSize = m_pGrid->GetCellSize();
  const ezVec3 vCellSize2 (vCellSize.x, 0.1f, vCellSize.z);

  for (ezUInt32 slice = 0; slice < m_pGrid->GetSlices(); ++slice)
  {
    for (ezUInt32 z = 0; z < m_pGrid->GetDepth(); ++z)
    {
      for (ezUInt32 x = 0; x < m_pGrid->GetWidth(); ++x)
      {
        const GameCellData& cd = m_pGrid->GetCell(ezGridCoordinate(x, z, slice));

        if (cd.m_iCellType == 1)
          RenderCube(m_pGrid->GetCellOrigin(ezGridCoordinate(x, z, slice)), vCellSize);
        else
        {
          if (cd.m_bOccupied == 0)
          //if (!cd.m_bOccupied)
            RenderCube(m_pGrid->GetCellOrigin(ezGridCoordinate(x, z, slice)), vCellSize2);
          else
          {
            glColor3ub(0, cd.m_bOccupied % 255, 0);
            RenderCube(m_pGrid->GetCellOrigin(ezGridCoordinate(x, z, slice)), vCellSize2, false);
          }
        }
      }
    }
  }
}

