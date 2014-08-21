#include <PCH.h>
#include <RTS/Components/RevealerComponent.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Time/Stopwatch.h>
#include <RTS/Level.h>

EZ_BEGIN_COMPONENT_TYPE(RevealerComponent, ezComponent, 1, RevealerComponentManager);
EZ_END_COMPONENT_TYPE();

float RevealerComponent::g_fDefaultRadius = 15.0f;

RevealerComponent::RevealerComponent()
{
  m_fRadius = 0.0f;
  m_uiVisibilityValue = 100;
  m_bRaycasting = true;
}

struct TagCellData
{
  RevealerComponent* m_pRevealer;
  Level* m_pLevel;
  GameGrid* m_pGrid;
  ezUInt8 m_uiVisibilityValue;
  ezCallbackResult::Enum m_BlockedResult;
};

ezCallbackResult::Enum RevealerComponent::TagCellVisible(ezInt32 x, ezInt32 y, void* pPassThrough)
{
  TagCellData* tcd = (TagCellData*) pPassThrough;

  if (!tcd->m_pGrid->IsValidCellCoordinate(ezVec2I32(x, y)))
    return tcd->m_BlockedResult;

  tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_uiVisibility = tcd->m_uiVisibilityValue;

  if (tcd->m_pGrid->GetCell(ezVec2I32(x, y)).m_iCellType == 1)
    return tcd->m_BlockedResult;

  return ezCallbackResult::Continue;
}

static ezDynamicArray<ezUInt8> Temp;

void RevealerComponent::Update()
{
  TagCellData tcd;
  tcd.m_pRevealer = this;
  tcd.m_pLevel = (Level*) GetWorld()->GetUserData();
  tcd.m_pGrid = &tcd.m_pLevel->GetGrid();
  tcd.m_uiVisibilityValue = m_uiVisibilityValue;
  tcd.m_BlockedResult = m_bRaycasting ? ezCallbackResult::Stop : ezCallbackResult::Continue;

  const ezVec2I32 vPos = tcd.m_pGrid->GetCellAtWorldPosition(GetOwner()->GetLocalPosition());

  const float fRadius = m_fRadius > 0 ? m_fRadius : g_fDefaultRadius;

  if (tcd.m_pGrid->IsValidCellCoordinate(vPos))
  {
    // update what we see
    if (m_bRaycasting)
    {
      ez2DGridUtils::ComputeVisibleArea(vPos.x, vPos.y, (ezUInt16) fRadius, tcd.m_pGrid->GetGridWidth(), tcd.m_pGrid->GetGridHeight(), TagCellVisible, &tcd, &Temp);
    }
    else
    {
      ez2DGridUtils::RasterizeCircle(vPos.x, vPos.y, fRadius, TagCellVisible, &tcd);
    }
  }

}
