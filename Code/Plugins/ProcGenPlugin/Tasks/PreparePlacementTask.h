#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezProcGenInternal
{
  class PreparePlacementTask : public ezTask
  {
  public:
    PreparePlacementTask(const char* szName);
    ~PreparePlacementTask();

  private:
    virtual void Execute() override;
  };
}
