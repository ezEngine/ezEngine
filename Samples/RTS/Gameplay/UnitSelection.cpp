#include <PCH.h>
#include <RTS/General/Application.h>


void SampleGameApp::SelectUnit()
{
  ezVec2I32 iCell = GetPickedGridCell();

  if (!m_pLevel->GetGrid().IsValidCellCoordinate(iCell))
    return;

  const GameCellData& Cell = m_pLevel->GetGrid().GetCell(iCell);

  UnitComponent* pUnit;
  if (m_pLevel->GetWorld()->TryGetComponent<UnitComponent>(Cell.m_hUnit, pUnit))
  {
    //m_pSelectedUnits->Clear();
    m_pSelectedUnits->ToggleSelection(pUnit->GetOwner()->GetHandle());
  }
}
