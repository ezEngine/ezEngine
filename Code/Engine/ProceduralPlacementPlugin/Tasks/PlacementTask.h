#pragma once

#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezPPInternal
{

  class EZ_PROCEDURALPLACEMENTPLUGIN_DLL PlacementTask : public ezTask
  {
  public:
    PlacementTask();
    ~PlacementTask();

    ezArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    friend class ActiveTile;

    virtual void Execute() override;

    const Layer* m_pLayer;

    ezDynamicArray<PlacementPoint, ezAlignedAllocatorWrapper> m_InputPoints;
    ezDynamicArray<PlacementTransform, ezAlignedAllocatorWrapper> m_OutputTransforms;
  };

}
