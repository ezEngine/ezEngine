#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezProcGenInternal
{
  class PreparePlacementTask : public ezTask
  {
  public:
    PreparePlacementTask(PlacementTask* placementTask, const char* szName);
    ~PreparePlacementTask();

  private:
    friend class PlacementTile;

    PlacementTask* m_pPlacementTask = nullptr;

    virtual void Execute() override;
  };
}
