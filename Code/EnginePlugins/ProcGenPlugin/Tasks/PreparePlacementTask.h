#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>

namespace ezProcGenInternal
{
  class PreparePlacementTask final : public ezTask
  {
  public:
    PreparePlacementTask(PlacementData* pData, const char* szName);
    ~PreparePlacementTask();

    void Clear() {}

  private:
    friend class PlacementTile;

    PlacementData* m_pData = nullptr;

    virtual void Execute() override;
  };
} // namespace ezProcGenInternal
