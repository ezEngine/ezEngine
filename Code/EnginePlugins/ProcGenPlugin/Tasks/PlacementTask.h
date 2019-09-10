#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>

class ezPhysicsWorldModuleInterface;

namespace ezProcGenInternal
{
  class PlacementTask : public ezTask
  {
  public:
    PlacementTask(const char* szName);
    ~PlacementTask();

    void Clear();

    ezArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    ezArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    friend class PlacementTile;
    friend class PreparePlacementTask;

    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    const ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

    ezSharedPtr<const PlacementOutput> m_pOutput;
    ezInt32 m_iTileSeed = 0;
    ezBoundingBox m_TileBoundingBox;

    ezDynamicArray<ezSimdMat4f, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    ezDynamicArray<PlacementPoint, ezAlignedAllocatorWrapper> m_InputPoints;
    ezDynamicArray<PlacementTransform, ezAlignedAllocatorWrapper> m_OutputTransforms;
    ezDynamicArray<float> m_TempData;
    ezDynamicArray<ezUInt32> m_ValidPoints;

    ezExpressionVM m_VM;
  };
} // namespace ezPPInternal
