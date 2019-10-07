#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezProcGenInternal
{
  class PreparePlacementTask : public ezTask
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
}
