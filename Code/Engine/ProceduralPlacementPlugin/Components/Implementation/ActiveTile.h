#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>

class ezPhysicsWorldModuleInterface;

namespace ezPPInternal
{
  class ActiveTile
  {
  public:
    ActiveTile();
    ActiveTile(ActiveTile&& other);
    ~ActiveTile();

    void Initialize(const TileDesc& desc, const Layer* pLayer);
    void Deinitialize();

    bool IsValid() const;

    const TileDesc& GetDesc() const;
    const Layer* GetLayer() const;
    ezBoundingBox GetBoundingBox() const;
    ezColor GetDebugColor() const;
    ezArrayPtr<const PlacementTransform> GetObjectTransforms() const;

    void Update(ezPhysicsWorldModuleInterface* pPhysicsModule);

    bool IsFinished() const;

  private:
    TileDesc m_Desc;
    const Layer* m_pLayer;

    ezUniquePtr<PlacementTask> m_pPlacementTask;
    ezTaskGroupID m_TaskGroupId;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum m_State;
  };
}
