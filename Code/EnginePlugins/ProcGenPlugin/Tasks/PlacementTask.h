#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>

class ezPhysicsWorldModuleInterface;
class ezVolumeCollection;

namespace ezProcGenInternal
{
  class PlacementTask : public ezTask
  {
  public:
    PlacementTask(PlacementData* pData, const char* szName);
    ~PlacementTask();

    void Clear();

    ezArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    ezArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    PlacementData* m_pData = nullptr;

    ezDynamicArray<PlacementPoint, ezAlignedAllocatorWrapper> m_InputPoints;
    ezDynamicArray<PlacementTransform, ezAlignedAllocatorWrapper> m_OutputTransforms;
    ezDynamicArray<float> m_TempData;
    ezDynamicArray<ezUInt32> m_ValidPoints;

    ezExpressionVM m_VM;
  };
} // namespace ezPPInternal
