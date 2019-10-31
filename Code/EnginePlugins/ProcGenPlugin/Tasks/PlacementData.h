#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionFunctions.h>

class ezPhysicsWorldModuleInterface;
class ezVolumeCollection;

namespace ezProcGenInternal
{
  struct PlacementData
  {
    PlacementData();
    ~PlacementData();

    void Clear();
    
    const ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

    ezSharedPtr<const PlacementOutput> m_pOutput;
    ezInt32 m_iTileSeed = 0;
    ezBoundingBox m_TileBoundingBox;

    ezDynamicArray<ezSimdMat4f, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    ezDynamicArray<ezVolumeCollection> m_VolumeCollections;
    ezExpression::GlobalData m_GlobalData;
  };
} // namespace ezProcGenInternal
