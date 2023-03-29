#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <ProcGenPlugin/Declarations.h>

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
    const ezWorld* m_pWorld = nullptr;

    ezSharedPtr<const PlacementOutput> m_pOutput;
    ezUInt32 m_uiTileSeed = 0;
    ezBoundingBox m_TileBoundingBox;

    ezDynamicArray<ezSimdMat4f, ezAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;

    ezDeque<ezVolumeCollection> m_VolumeCollections;
    ezExpression::GlobalData m_GlobalData;
  };
} // namespace ezProcGenInternal
