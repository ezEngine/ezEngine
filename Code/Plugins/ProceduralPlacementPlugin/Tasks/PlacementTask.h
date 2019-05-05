#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>
#include <ProceduralPlacementPlugin/VM/ExpressionVM.h>

class ezPhysicsWorldModuleInterface;

namespace ezPPInternal
{
  class EZ_PROCEDURALPLACEMENTPLUGIN_DLL PlacementTask : public ezTask
  {
  public:
    PlacementTask(const char* szName);
    ~PlacementTask();

    void Clear();

    ezArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    ezArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    friend class ActiveTile;
    friend class PrepareTask;

    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    const ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

    ezSharedPtr<const Layer> m_pLayer;
    ezInt32 m_iTileSeed = 0;
    ezBoundingBox m_TileBoundingBox;

    ezDynamicArray<ezSimdTransform, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    ezDynamicArray<PlacementPoint, ezAlignedAllocatorWrapper> m_InputPoints;
    ezDynamicArray<PlacementTransform, ezAlignedAllocatorWrapper> m_OutputTransforms;
    ezDynamicArray<float> m_TempData;
    ezDynamicArray<ezUInt32> m_ValidPoints;

    ezExpressionVM m_VM;
  };
} // namespace ezPPInternal
