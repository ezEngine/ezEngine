#pragma once

#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>
#include <ProceduralPlacementPlugin/VM/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezPPInternal
{
  class EZ_PROCEDURALPLACEMENTPLUGIN_DLL PrepareTask : public ezTask
  {
  public:
    PrepareTask(const char* szName);
    ~PrepareTask();

  private:
    virtual void Execute() override;
  };
}
